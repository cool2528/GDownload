#include "aria2c_manager.h"
#include <cpr/cpr.h>
#include <boost/url.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <chrono>
#include <thread>
#include <set>
#include "engine_def.h"
#include "logger.h"
#include "os/os.h"
#include "process/process.h"
#include "cache/cache.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "aria2c_http_rpc_client.h"
#include "config/config.h"
namespace gdl {
	namespace engine {

		namespace detail {
			std::string GetValidRpcPort() {
				auto rpc_port = config::GetValue(config::Keys::RpcListenPort).AsString();
				int port_value = 0;
				try {
					port_value = std::stoi(rpc_port);
				} catch (...) {
					port_value = 0;
				}
				if (port_value < 1024 || port_value > 65535) {
					return kEngineRpcPort;
				}
				return rpc_port;
			}

			std::string GetRpcSecret() {
				auto rpc_secret = config::GetValue(config::Keys::RpcSecret).AsString();
				if (rpc_secret.empty()) {
					rpc_secret = kDefaultRpcSecret;
				}
				return rpc_secret;
			}

			std::string KilobytesPerSecondToBytesPerSecond(config::ConfigValue value) {
				return std::to_string(static_cast<std::int64_t>(value.AsInt()) * 1024);
			}

			nlohmann::json DefaultTrackerSourceNames() {
				return nlohmann::json::array({"ngosang-best-link",
											  "ngosang-best-mirror",
											  "ngosang-best-cdn",
											  "ngosang-all-link",
											  "ngosang-all-mirror",
											  "ngosang-all-cdn",
											  "ngosang-all_udp-link",
											  "ngosang-all_udp-mirror",
											  "ngosang-all_udp-cdn",
											  "ngosang-all_http-link",
											  "ngosang-all_http-mirror",
											  "ngosang-all_http-cdn",
											  "ngosang-all_https-link",
											  "ngosang-all_https-mirror",
											  "ngosang-all_https-cdn",
											  "XIU2-best-link",
											  "XIU2-best-cdn",
											  "XIU2-all-link",
											  "XIU2-all-cdn",
											  "XIU2-http-link",
											  "XIU2-http-cdn",
											  "XIU2-nohttp-link",
											  "XIU2-nohttp-cdn"});
			}
		}  // namespace detail
		Aria2cDownloadManager::Aria2cDownloadManager()
			: work_(boost::asio::make_work_guard(io_context_)),
			  daily_task_timer_(io_context_),
			  update_aria2c_tasks_timer_(io_context_),
			  pub_sub_system_(io_context_),
			  websocket_client_([]() {
			auto rpc_port = detail::GetValidRpcPort();
			return std::string("ws://127.0.0.1:") + rpc_port + "/jsonrpc";
			  }(), io_context_) {

			websocket_client_.SetMessageCallback([this](const std::string& msg) {
				// Forward direct messages to clients through pub/sub.
				try {
					std::string_view msg_view(msg);
					nlohmann::json doc = nlohmann::json::parse(msg_view, nullptr, false);
					if (doc.contains("error")) {
						const auto& error = doc["error"];
						if (error.is_object() && error.contains("code")) {
							if (error["code"].get<std::int64_t>() == -32600) {
								return;
							}
						}
					}
					if (doc.contains("result")) {
						const auto& result = doc["result"];
						if (result.contains("numActive") && result.contains("numWaiting") &&
							result.contains("numStopped")) {
							active_num_	 = std::stoll(result["numActive"].get<std::string>());
							waiting_num_ = std::stoll(result["numWaiting"].get<std::string>());
							stopped_num_ = std::stoll(result["numStopped"].get<std::string>());
							return;
						}
					}

					pub_sub_system_.Publish(kAria2Response, msg);

				} catch (const nlohmann::json::exception& e) {
					LOG_ERR("JSON parse error: {} for data: {}", e.what(), msg);
				} catch (const std::exception& e) {
					LOG_ERR("General error: {} for data: {}", e.what(), msg);
				}
			});
			websocket_client_.SetStateChanageCallback([this](const State& state, std::string msg) {
				// 按连接状态分派：首次连接才启动每日 tracker 同步，重连/正常关闭不再误报 WARN（B6）
				switch (state) {
					case State::kConnected:
						if (!daily_task_timer_is_runing.load()) {
							LOG_DBG("start daily_task_timer");
							daily_task_timer_.Start([this] { DispatchMagnetServerSync(); });
							daily_task_timer_is_runing.store(true);
						}
						else {
							LOG_DBG("websocket reconnected");
						}
						break;
					case State::kClosed:
						LOG_INFO("websocket closed: {}", msg);
						break;
					case State::kError:
						LOG_WARN("websocket error: {}", msg);
						break;
				}
			});
			// Start IO workers after callbacks are registered.
			auto hardware_threads = std::max(1u, std::thread::hardware_concurrency());
			auto max_thread_number = std::max(1u, hardware_threads * 3 / 2);
			for (auto i = 0u; i < max_thread_number; ++i) {
				worker_threads_.emplace_back(std::thread([this] { io_context_.run(); }));
			}
		}

		std::vector<String> Aria2cDownloadManager::InitAria2cSettingsArgs() {
			std::vector<String> result;
			// Arguments are passed as argv; process::Execute handles spaces and quoting.
			auto quote_path = [](const String& path) {
				return path;
			};
			std::unordered_map<std::string, std::string> aria2c_settings;
			aria2c_settings["no-conf"]				  = "false";  //no-conf
            aria2c_settings["conf-path"]			  = quote_path(config::GetValue(config::Keys::ConfPath).AsString());
			aria2c_settings["allow-overwrite"]		  = "false";
			aria2c_settings["auto-file-renaming"]	  = "true";
			aria2c_settings["bt-exclude-tracker"]	  = "";
			aria2c_settings["bt-force-encryption"]	  = "false";
			aria2c_settings["bt-load-saved-metadata"] = "true";
			aria2c_settings["bt-save-metadata"]		  = "true";
			aria2c_settings["bt-tracker"]			  = "";
            aria2c_settings["continue"]				  = config::GetValue(config::Keys::AutoResumeTask).AsString();
			aria2c_settings["dht-file-path"]		  = quote_path(GetDhtPath(IP_VERSION::V4));
			aria2c_settings["dht-file-path6"]		  = quote_path(GetDhtPath(IP_VERSION::V6));

			aria2c_settings["dht-listen-port"] = config::GetValue(config::Keys::DhtListenPort).AsString();	//"26701";
			aria2c_settings["dir"]			   = config::GetValue(config::Keys::Dir).AsString();
			aria2c_settings["enable-dht6"]	   = "true";
			aria2c_settings["follow-metalink"] = "true";
			aria2c_settings["follow-torrent"]  = "true";
			aria2c_settings["listen-port"]	   = config::GetValue(config::Keys::ListenPort).AsString();	 //"21301"
			aria2c_settings["max-concurrent-downloads"]	  = config::GetValue(config::Keys::MaxConcurrentDownloads).AsString();
			aria2c_settings["max-connection-per-server"]  = config::GetValue(config::Keys::MaxConnectionPerServer).AsString();
			aria2c_settings["max-download-limit"]		  = detail::KilobytesPerSecondToBytesPerSecond(config::GetValue(config::Keys::MaxDownloadLimit));
			aria2c_settings["max-overall-download-limit"] = detail::KilobytesPerSecondToBytesPerSecond(config::GetValue(config::Keys::MaxOverallDownloadLimit));
			aria2c_settings["max-upload-limit"]			  = detail::KilobytesPerSecondToBytesPerSecond(config::GetValue(config::Keys::MaxUploadLimit));
			aria2c_settings["max-overall-upload-limit"]	  = detail::KilobytesPerSecondToBytesPerSecond(config::GetValue(config::Keys::MaxOverallUploadLimit));
			aria2c_settings["lowest-speed-limit"]		  = detail::KilobytesPerSecondToBytesPerSecond(config::GetValue(config::Keys::LowestSpeedLimit));
			aria2c_settings["min-split-size"]			  = config::GetValue(config::Keys::MinSplitSize).AsString() + "M";
			aria2c_settings["timeout"]					  = config::GetValue(config::Keys::Timeout).AsString();
			aria2c_settings["connect-timeout"]			  = config::GetValue(config::Keys::ConnectTimeout).AsString();
			aria2c_settings["max-tries"]				  = config::GetValue(config::Keys::MaxTries).AsString();
			aria2c_settings["retry-wait"]				  = config::GetValue(config::Keys::RetryWait).AsString();
			aria2c_settings["enable-dht"]				  = config::GetValue(config::Keys::EnableDht).AsString();
			aria2c_settings["bt-max-peers"]				  = config::GetValue(config::Keys::BtMaxPeers).AsString();
			aria2c_settings["bt-require-crypto"]		  = config::GetValue(config::Keys::BtRequireCrypto).AsString();
			aria2c_settings["no-proxy"]					  = "";
			aria2c_settings["pause-metadata"]			  = "false";
			aria2c_settings["pause"]					  = "false";
			aria2c_settings["rpc-listen-port"]			  = detail::GetValidRpcPort();
			aria2c_settings["rpc-secret"]				  = detail::GetRpcSecret();
			aria2c_settings["seed-ratio"]				  = "2";
			aria2c_settings["seed-time"]				  = "2880";
			aria2c_settings["split"]					  = config::GetValue(config::Keys::Split).AsString();
			auto user_agent = config::GetValue(config::Keys::UserAgent).AsString();
			if (user_agent.empty()) {
				user_agent =
					"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) "
					"Chrome/111.0.0.0 Safari/537.36";
			}
			aria2c_settings["user-agent"] = quote_path(user_agent);
			aria2c_settings["check-certificate"] = "true";
#if (defined(DEBUG) || defined(_DEBUG))
			aria2c_settings["quiet"] = "false";
#else
			aria2c_settings["quiet"] = "true";
#endif
			aria2c_settings["enable-rpc"]			= "true";
			aria2c_settings["rpc-listen-all"]		= "false";
			aria2c_settings["rpc-allow-origin-all"] = "false";
            auto is_save_session					= config::GetValue(config::Keys::IsSaveSession).AsBool();
            auto session_path						= config::GetValue(config::Keys::SaveSession).AsString();
            if (is_save_session) aria2c_settings["save-session"] = quote_path(session_path);
            std::error_code ec;
            if (std::filesystem::exists(session_path, ec) && std::filesystem::file_size(session_path) != 0) {
                aria2c_settings["input-file"] = quote_path(session_path);
            }
            // Check whether the global proxy is enabled.
            auto is_proxy_enable	 = config::GetValue(config::Keys::EnableGlobalProxy).AsBool();
            auto global_proxy_string = config::GetValue(config::Keys::GlobalProxy).AsString();
            if (is_proxy_enable && !global_proxy_string.empty()) {
                aria2c_settings["all-proxy"] = global_proxy_string;
            }
#ifdef _WIN32
			DWORD processId = GetCurrentProcessId();
#else
			pid_t processId = getpid();
#endif
			aria2c_settings["stop-with-process"] = std::to_string(processId);
			for (const auto& pair : aria2c_settings) {
				const auto key	 = "--" + pair.first;
				const auto value = "=" + pair.second;
				result.emplace_back(key + value);
			}
			return result;
		}

		String Aria2cDownloadManager::GetDhtPath(IP_VERSION protocol) {
			const String name = protocol == IP_VERSION::V4 ? "dht.dat" : "dht6.dat";
			return os::GetAppDataDir() + "/gdownload/" + name;
		}

		void Aria2cDownloadManager::UpdateAria2cTasks() {
			// Skip updates during shutdown to avoid racing with Shutdown/Purge calls.
			if (!engine_is_runing_) return;
			// Refresh aria2 task state lists.
			static const std::vector<std::string> keys = {
				"status", "totalLength", "completedLength", "downloadSpeed", "infoHash", "numSeeders",
				"seeder", "connections", "errorCode",		"errorMessage",	 "dir",		 "files",
				"gid",	  "bittorrent"};
			try {
				SyncGlobalStatInfo();
				if (active_num_ > 0) {
					websocket_client_.TellActive(keys);
                    auto active_all_progress = SyncActiveTaskInfo();
                    pub_sub_system_.Publish(kAria2ActiveProgress, active_all_progress);
                }
                else {
                    nlohmann::json active_tasks;
                    active_tasks["totalLength"]		= 0;
                    active_tasks["completedLength"] = 0;
                    pub_sub_system_.Publish(kAria2ActiveProgress, active_tasks.dump());
                }
				if (waiting_num_ > 0) {
					websocket_client_.TellWaiting(0, 100, keys);
				}
				// 放宽守卫：纯已完成会话重启后也能刷新已停止列表（B5）
				if (stopped_num_ > 0) {
					websocket_client_.TellStopped(0, 100, keys);
				}

			} catch (std::exception& e) {
				LOG_ERR("{}", e.what());
			}
		}

		void Aria2cDownloadManager::SyncMagnetServerList() {
			auto start_time = std::chrono::steady_clock::now();

			// Publish start status.
			pub_sub_system_.Publish(kAria2TrackerUpdateStatus,
			                        R"({"status":"started","message":"Updating tracker list..."})");

			try {
				// 1. Download blacklist.
				const std::string trackers_black_url(
					"https://bitbucket.org/xiu2/trackerslistcollection/raw/master/blacklist.txt");
				auto bt_exclude_tracker = GetBitTorrentUrlWithFallback(trackers_black_url);
				if (!engine_is_runing_) return;

				if (!bt_exclude_tracker.empty()) {
					websocket_client_.ChangeGlobalOption({{"bt-exclude-tracker", bt_exclude_tracker}});
					LOG_INFO("Updated tracker blacklist: {} trackers", CountTrackers(bt_exclude_tracker));
				} else {
					LOG_WARN("Failed to fetch tracker blacklist");
				}

				// 2. Load configured sources and fall back to defaults on invalid config.
				auto json_data = config::GetValue(config::Keys::TrackerSourceNames).AsString();
				nlohmann::json tracker_source_names = nlohmann::json::parse(json_data.c_str(), nullptr, false);
				if (tracker_source_names.is_discarded() || !tracker_source_names.is_array()) {
					tracker_source_names = detail::DefaultTrackerSourceNames();
				}

				// 3. Deduplicate trackers.
				std::unordered_set<std::string> unique_trackers;
				int successful_sources = 0;
				int failed_sources = 0;

				for (const auto& key : tracker_source_names) {
					if (!key.is_string() || !engine_is_runing_) break;

					std::string name = key.get<std::string>();
					auto source_url = config::GetTrackersServerUrl(name);

					LOG_INFO("Fetching trackers from source: {}", name);
					auto bt_tracker_urls = GetBitTorrentUrlWithFallback(source_url);

					if (!bt_tracker_urls.empty()) {
						// Split and insert into the set for deduplication.
						std::istringstream stream(bt_tracker_urls);
						std::string url;
						while (std::getline(stream, url, ',')) {
							if (!url.empty()) {
								unique_trackers.insert(url);
							}
						}
						successful_sources++;
						LOG_INFO("Source {} provided {} trackers", name, CountTrackers(bt_tracker_urls));
					} else {
						failed_sources++;
						LOG_WARN("Failed to fetch from source: {}", name);
					}
				}

				// 4. Convert to comma-separated string.
				std::string bt_tracker;
				for (const auto& url : unique_trackers) {
					if (!engine_is_runing_) return;
					if (!bt_tracker.empty()) bt_tracker += ",";
					bt_tracker += url;
				}

				auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - start_time);

				if (!bt_tracker.empty()) {
					// 5. Sync to aria2c.
					websocket_client_.ChangeGlobalOption({{"bt-tracker", bt_tracker}});

					// 6. Publish success notification.
					std::string bt_tracker_display = bt_tracker;
					std::replace(bt_tracker_display.begin(), bt_tracker_display.end(), ',', '\n');
					pub_sub_system_.Publish(kAria2SyncMagnetServerList, bt_tracker_display);

					nlohmann::json success_msg;
					success_msg["status"] = "success";
					success_msg["message"] = "Tracker list updated successfully";
					success_msg["tracker_count"] = static_cast<int>(unique_trackers.size());
					success_msg["successful_sources"] = successful_sources;
					success_msg["failed_sources"] = failed_sources;
					success_msg["elapsed_ms"] = elapsed.count();
					pub_sub_system_.Publish(kAria2TrackerUpdateStatus, success_msg.dump());

					LOG_INFO("Tracker update completed: {} unique trackers from {}/{} sources in {}ms",
					         unique_trackers.size(), successful_sources,
					         successful_sources + failed_sources, elapsed.count());
				} else {
					throw std::runtime_error("No trackers fetched from any source");
				}

			} catch (const std::exception& e) {
				LOG_ERR("Failed to sync tracker list: {}", e.what());

				nlohmann::json error_msg;
				error_msg["status"] = "error";
				error_msg["message"] = "Failed to update tracker list";
				error_msg["error"] = e.what();
				pub_sub_system_.Publish(kAria2TrackerUpdateStatus, error_msg.dump());
			}
		}

		void Aria2cDownloadManager::DispatchMagnetServerSync() {
			// 上一次同步未结束则跳过，避免并发重复拉取
			bool expected = false;
			if (!tracker_sync_running_.compare_exchange_strong(expected, true)) {
				return;
			}
			// 阻塞式 HTTP + 退避 sleep 放到独立线程，避免占死 io worker（E3）
			tracker_sync_future_ = std::async(std::launch::async, [this] {
				SyncMagnetServerList();
				tracker_sync_running_.store(false);
			});
		}

		void Aria2cDownloadManager::SyncGlobalStatInfo() {
		// Engine layer uses the config system.
		auto rpc_port = config::GetValue(config::Keys::RpcListenPort).AsString();
		// Validate port range; use default when empty or invalid.
		int port_value = 0;
		try {
			port_value = std::stoi(rpc_port);
		} catch (...) {
			port_value = 0;
		}
		if (port_value < 1024 || port_value > 65535) {
			rpc_port = kEngineRpcPort;
		}
		const std::string host = std::string("http://127.0.0.1:") + rpc_port;
			Aria2cHttpClient client(host);
			auto http_result = client.GetGlobalStat();
			if (auto res = std::get_if<ErrorResult>(&http_result.Value().result)) {
				LOG_WARN("SyncGlobalStatInfo fail: {}", res->err_msg)
				return;
			}
			else if (auto succeed_res = std::get_if<SucceedResult>(&http_result.Value().result)) {
				nlohmann::json doc = nlohmann::json::parse(succeed_res->body, nullptr, false);
				if (doc.contains("result")) {
					const auto& result = doc["result"];
					if (result.contains("numActive") && result.contains("numWaiting") &&
						result.contains("numStopped")) {
						active_num_	 = std::stoll(result["numActive"].get<std::string>());
						waiting_num_ = std::stoll(result["numWaiting"].get<std::string>());
						stopped_num_ = std::stoll(result["numStopped"].get<std::string>());
					}
				}
			}
		}

        std::string Aria2cDownloadManager::SyncActiveTaskInfo() {
		std::string result_string				   = "{}";
		static const std::vector<std::string> keys = {"status", "totalLength", "completedLength"};
		// Engine layer uses the config system.
		auto rpc_port = config::GetValue(config::Keys::RpcListenPort).AsString();
		// Validate port range; use default when empty or invalid.
		int port_value = 0;
		try {
			port_value = std::stoi(rpc_port);
		} catch (...) {
			port_value = 0;
		}
		if (port_value < 1024 || port_value > 65535) {
			rpc_port = kEngineRpcPort;
		}
		const std::string host					   = std::string("http://127.0.0.1:") + rpc_port;
            Aria2cHttpClient client(host);
            auto http_result = client.TellActive(keys);
            if (auto res = std::get_if<ErrorResult>(&http_result.Value().result)) {
                LOG_WARN("SyncActiveTaskInfo fail: {}", res->err_msg)
                return result_string;
            }
            else if (auto succeed_res = std::get_if<SucceedResult>(&http_result.Value().result)) {
                nlohmann::json doc = nlohmann::json::parse(succeed_res->body, nullptr, false);
                if (doc.contains("result") && doc["result"].is_array()) {
                    const auto& result					= doc["result"];
                    std::int64_t active_totalLength		= 0;
                    std::int64_t active_completedLength = 0;
                    nlohmann::json active_tasks;
                    for (const auto& object_doc : result) {
                        if (object_doc.contains("totalLength") && object_doc.contains("completedLength") &&
                            object_doc.contains("status")) {
                            active_totalLength += std::stoll(object_doc["totalLength"].get<std::string>());
                            active_completedLength += std::stoll(object_doc["completedLength"].get<std::string>());
                        }
                    }
                    active_tasks["totalLength"]		= active_totalLength;
                    active_tasks["completedLength"] = active_completedLength;
                    result_string					= active_tasks.dump();
                }
            }
            return result_string;
        }

		std::string Aria2cDownloadManager::ParseTextUrls(const std::string& inputText) {
			std::vector<std::string> result;
			std::istringstream input(inputText);
			std::string line;
			while (std::getline(input, line)) {
				line.erase(0, line.find_first_not_of(" \t"));
				line.erase(line.find_last_not_of(" \t") + 1);
				if (line.empty() || line[0] == '#') {
					continue;
				}
				result.push_back(line);
			}
			std::ostringstream oss;
			for (size_t i = 0; i < result.size(); ++i) {
				if (i > 0) {
					oss << ",";
				}
				oss << result[i];
			}
			return oss.str();
		}


		int Aria2cDownloadManager::CountTrackers(const std::string& tracker_list) {
			if (tracker_list.empty()) return 0;
			return static_cast<int>(std::count(tracker_list.begin(), tracker_list.end(), ',')) + 1;
		}

		void Aria2cDownloadManager::InitializeETagCache() {
			auto db_path = os::GetAppDataDir() + "/gdownload/tracker_etag_cache.db";
			if (!cache::TrackerETagCache::Instance().Initialize(db_path)) {
				LOG_ERR("Failed to initialize TrackerETagCache database");
			}
		}


		void Aria2cDownloadManager::UpdateCacheEntry(const std::string& url, const std::string& etag,
													  const std::string& content) {
			cache::TrackerETagEntry entry;
			entry.url = url;
			entry.etag = etag;
			entry.content = content;
			entry.timestamp =
				std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
					.count();

			if (cache::TrackerETagCache::Instance().SetEntry(entry)) {
				LOG_INFO("Updated ETag cache for: {} (ETag: {})", url, etag);
			} else {
				LOG_ERR("Failed to update ETag cache for: {}", url);
			}
		}



		std::string Aria2cDownloadManager::ConvertToJsDelivrCDN(const std::string& url) {
			// https://raw.githubusercontent.com/user/repo/branch/file
			// -> https://cdn.jsdelivr.net/gh/user/repo@branch/file
			std::string cdn_url = url;
			size_t pos = cdn_url.find("raw.githubusercontent.com");

			if (pos != std::string::npos) {
				// Replace host.
				cdn_url.replace(pos, 25, "cdn.jsdelivr.net/gh");

				// Replace the separator after user/repo with '@'.
				// Format: https://cdn.jsdelivr.net/gh/user/repo/branch/file
				//                                      ^1   ^2  ^3
				size_t start_pos = cdn_url.find("gh/") + 3;
				int slash_count = 0;
				for (size_t i = start_pos; i < cdn_url.size(); ++i) {
					if (cdn_url[i] == '/') {
						slash_count++;
						if (slash_count == 2) {  // First slash after user/repo.
							cdn_url[i] = '@';
							break;
						}
					}
				}
			}

			return cdn_url;
		}

		std::string Aria2cDownloadManager::GetBitTorrentUrlWithFallback(const std::string& url) {
			if (url.empty() || !engine_is_runing_) return "";

			// Build fallback URL list.
			std::vector<std::string> fallback_urls = {url};

			// Add CDN mirrors for GitHub raw URLs.
			if (url.find("raw.githubusercontent.com") != std::string::npos) {
				fallback_urls.push_back(ConvertToJsDelivrCDN(url));
			}

			// Get system proxy.
			auto system_proxy = os::GetSystemHTTPProxy();
			std::string proxy_str;
			if (system_proxy.has_value()) {
				proxy_str = "http://" + system_proxy.value().first + ":" +
				            std::to_string(system_proxy.value().second);
			}

			// Try each URL with retries.
			for (const auto& try_url : fallback_urls) {
				if (!engine_is_runing_) return "";

				// Check cached ETag from database.
				std::string cached_etag;
				auto cached_entry = cache::TrackerETagCache::Instance().GetEntry(try_url);
				if (cached_entry) {
					cached_etag = cached_entry->etag;
					LOG_INFO("Found cached ETag for: {} (ETag: {})", try_url, cached_etag);
				}

				for (int retry = 0; retry < 3; ++retry) {
					if (!engine_is_runing_) return "";
					try {
						LOG_INFO("Fetching from: {} (attempt {})", try_url, retry + 1);

						// Build request headers.
						cpr::Header headers;
						if (!cached_etag.empty()) {
							headers["If-None-Match"] = cached_etag;
							LOG_INFO("Sending If-None-Match: {}", cached_etag);
						}

						// 无系统代理时不设置 cpr::Proxies，避免空代理串导致请求失败（B4）
						cpr::Response reply;
						if (proxy_str.empty()) {
							reply = cpr::Get(cpr::Url(try_url), cpr::Timeout(10000), headers);
						} else {
							reply = cpr::Get(cpr::Url(try_url),
							                 cpr::Proxies({{"http", proxy_str}, {"https", proxy_str}}),
							                 cpr::Timeout(10000), headers);
						}

						// Handle 304 Not Modified by using cached content.
						if (reply.status_code == 304) {
							if (cached_entry) {
								LOG_INFO("HTTP 304 Not Modified, using cached content for: {}", try_url);
								return cached_entry->content;
							} else {
								LOG_WARN("HTTP 304 but no cached content found for: {}", try_url);
								continue;
							}
						}

						// Handle 200 OK by saving content and ETag.
						if (reply.status_code == 200) {
							auto result = ParseTextUrls(reply.text);

							// Extract and save ETag.
							auto etag_it = reply.header.find("etag");
							if (etag_it != reply.header.end()) {
								std::string new_etag = etag_it->second;
								LOG_INFO("Received new ETag: {} for: {}", new_etag, try_url);
								UpdateCacheEntry(try_url, new_etag, result);
							} else {
								LOG_INFO("No ETag in response for: {}", try_url);
							}

							LOG_INFO("Successfully fetched {} trackers from: {}", CountTrackers(result), try_url);
							return result;
						}

						LOG_WARN("HTTP {} from: {}", reply.status_code, try_url);

					} catch (const std::exception& e) {
						LOG_WARN("Exception fetching from {}: {}", try_url, e.what());
					}

					// Wait before retrying with exponential backoff: 1s, 2s.
					if (retry < 2) {
						const auto delay = std::chrono::seconds(1 << retry);
						const auto deadline = std::chrono::steady_clock::now() + delay;
						while (engine_is_runing_ && std::chrono::steady_clock::now() < deadline) {
							const auto remaining = deadline - std::chrono::steady_clock::now();
							std::this_thread::sleep_for(std::min<std::chrono::steady_clock::duration>(
								remaining, std::chrono::milliseconds(100)));
						}
					}
				}
			}

			LOG_ERR("All fallback URLs failed for: {}", url);
			return "";
		}

		Aria2cDownloadManager::~Aria2cDownloadManager() {
			// 兜底：任何未显式调用 UninitAria2cEngine 的退出路径也能停 io、join 线程（E4）
			UninitAria2cEngine();
		}

		bool Aria2cDownloadManager::InitAria2cEngine(const String_View& aria2c_path) {
			// TODO: initialize aria2c.
			aria2c_path_			 = String(aria2c_path);
			std::vector<String> args = InitAria2cSettingsArgs();
			auto pid				 = process::Execute(aria2c_path, args);
			if (pid <= 0) {
				LOG_ERR("Failed to initialise aria2c Failed to start the process");
				return false;
			}
			aria2c_pid_ = pid;  // 登记 pid，退出时优雅关闭并回收（S3）

			// Initialize ETag cache database.
			InitializeETagCache();

			// Start task polling. Tracker auto-update must not affect task list refresh.
			update_aria2c_tasks_timer_.Start(std::bind(&Aria2cDownloadManager::UpdateAria2cTasks, this),
											 std::chrono::milliseconds(300), true);
			// Connect aria2c websocket.
			websocket_client_.Open();
			engine_is_runing_ = true;

			return true;
		}

		void Aria2cDownloadManager::UninitAria2cEngine() {
			// 幂等化：显式 Uninit 与析构兜底可能重复调用，二次进入直接返回（E4）
			bool expected = false;
			if (!uninited_.compare_exchange_strong(expected, true)) {
				return;
			}
			engine_is_runing_ = false;
			// 等待可能在途的 tracker 同步线程结束（内部循环见 engine_is_runing_ 会尽快退出）（E3）
			if (tracker_sync_future_.valid()) {
				tracker_sync_future_.wait();
			}
			update_aria2c_tasks_timer_.Stop();
			daily_task_timer_.Stop();
			websocket_client_.PurgeDownloadResult();
			websocket_client_.Shutdown();
			// 给 RPC/stop-with-process 优雅退出宽限，超时强制终止并回收，避免 POSIX 僵尸（S3）
			if (aria2c_pid_ > 0) {
				process::ShutdownProcess(aria2c_pid_, 2000);
			} else {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			websocket_client_.Disconnect();
			work_.reset();
			io_context_.stop();
			for (auto& t : worker_threads_) {
				if (t.joinable()) {
					t.join();
				}
			}
		}

		Result<bool> Aria2cDownloadManager::AddHttpTask(
            const std::vector<String>& url, const std::unordered_multimap<std::string, std::string>& options) {

			std::unordered_multimap<std::string, std::string> real_options = options;
			// Add directly to the download engine queue.
            return websocket_client_.AddUri(url, real_options);
		}

		Result<bool> Aria2cDownloadManager::AddTorrentTask(
			const String& tarrent, const std::unordered_multimap<std::string, std::string>& options) {
			return websocket_client_.AddTorrent(tarrent, options);
		}

		Result<bool> Aria2cDownloadManager::AddMetalinkTask(
			const String& metalink, const std::unordered_multimap<std::string, std::string>& options) {
			return websocket_client_.AddMetalink(metalink, options);
		}

		Result<Subscription> Aria2cDownloadManager::SubscriptionAria2Message(
			const std::string& topic, std::function<void(const std::string&)> handler) {
			auto sub = pub_sub_system_.Subscribe(topic, handler);
			if (sub) {
				return sub;
			}
			return MakeFail(1, "Subscribe failed");
		}

		void Aria2cDownloadManager::UpdateMagnetServerList() {
			DispatchMagnetServerSync();
		}

		void Aria2cDownloadManager::UnSubscribeAria2Message(Subscription subscription) {
			return pub_sub_system_.Unsubscribe(subscription);
		}

	}  // namespace engine

}  // namespace gdl

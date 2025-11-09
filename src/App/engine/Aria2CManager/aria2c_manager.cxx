#include "aria2c_manager.h"
#include <cpr/cpr.h>
#include <boost/url.hpp>
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <sstream>
#include <chrono>
#include <thread>
#include <set>
#include <random>
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

		namespace detail {}
		Aria2cDownloadManager::Aria2cDownloadManager()
			: work_(boost::asio::make_work_guard(io_context_)),
			  daily_task_timer_(io_context_),
			  update_aria2c_tasks_timer_(io_context_),
			  pub_sub_system_(io_context_),
			  flush_timer_(io_context_),
			  websocket_client_([]() {
				  auto rpc_port = config::GetValue(config::Keys::RpcListenPort).AsString();
				  if (rpc_port.empty()) rpc_port = kEngineRpcPort;
				  return std::string("ws://127.0.0.1:") + rpc_port + "/jsonrpc";
			  }(), io_context_) {

			websocket_client_.SetMessageCallback([this](const std::string& msg) {
				// 直接收到的所有消息 通过发布订阅回客户端
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

					pub_sub_system_.Publish(kAria2Responce, msg);

				} catch (const nlohmann::json::exception& e) {
					LOG_ERR("JSON parse error: {} for data: {}", e.what(), msg);
				} catch (const std::exception& e) {
					LOG_ERR("General error: {} for data: {}", e.what(), msg);
				}
			});
			websocket_client_.SetStateChanageCallback([this](const State& state, std::string msg) {
				// 只有web socket 链接上 aria2c 服务端后才去同步BitTorrent 服务器列表这样才能设置上去
				if (!daily_task_timer_is_runing.load() && state == State::kConnected) {
					LOG_DBG("start daily_task_timer");
					daily_task_timer_.Start(std::bind(&Aria2cDownloadManager::SyncMagnetServerList, this));
					daily_task_timer_is_runing.store(true);
				}
				else {
					LOG_WARN("websocket_client_ state {} error {}", static_cast<int>(state), msg);
				}
			});
			// io 线程放在最后
			auto max_thread_number = std::thread::hardware_concurrency() * 3 / 2;
			for (auto i = 0; i < max_thread_number; ++i) {
				worker_threads_.emplace_back(std::thread([this] { io_context_.run(); }));
			}
		}

		std::vector<String> Aria2cDownloadManager::InitAria2cSettingsArgs() {
			std::vector<String> result;
			// 处理包含空格的路径
			auto quote_path = [](const String& path) {
#if defined(_WIN32) || defined(_WIN64)
				return "\"" + path + "\"";
#else
				return path;
#endif
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
			aria2c_settings["max-concurrent-downloads"]	  = "5";
			aria2c_settings["max-connection-per-server"]  = "64";
			aria2c_settings["max-download-limit"]		  = "0";
			aria2c_settings["max-overall-download-limit"] = "0";
			aria2c_settings["max-overall-upload-limit"]	  = "0";
			aria2c_settings["no-proxy"]					  = "";
			aria2c_settings["pause-metadata"]			  = "false";
			aria2c_settings["pause"]					  = "false";
			// 使用配置的 RPC 监听端口，如果配置为空则使用默认端口
			auto rpc_port = config::GetValue(config::Keys::RpcListenPort).AsString();
			aria2c_settings["rpc-listen-port"] = rpc_port.empty() ? kEngineRpcPort : rpc_port;
			// 使用配置的 RPC Secret，如果配置为空则使用默认值
			auto rpc_secret = config::GetValue(config::Keys::RpcSecret).AsString();
			aria2c_settings["rpc-secret"]				  = rpc_secret.empty() ? kDefaultRpcSecret : rpc_secret;
			aria2c_settings["seed-ratio"]				  = "2";
			aria2c_settings["seed-time"]				  = "2880";
			aria2c_settings["split"]					  = "64";
			aria2c_settings["user-agent"]				  = quote_path(
				"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) "
								"Chrome/111.0.0.0 Safari/537.36");
			aria2c_settings["check-certificate"] = "false";
#if (defined(DEBUG) || defined(_DEBUG))
			aria2c_settings["quiet"] = "false";
#else
			aria2c_settings["quiet"] = "true";
#endif
			aria2c_settings["enable-rpc"]			= "true";
			aria2c_settings["rpc-listen-all"]		= "true";
			aria2c_settings["rpc-allow-origin-all"] = "true";
            auto is_save_session					= config::GetValue(config::Keys::IsSaveSession).AsBool();
            auto session_path						= config::GetValue(config::Keys::SaveSession).AsString();
            if (is_save_session) aria2c_settings["save-session"] = quote_path(session_path);
            std::error_code ec;
            if (std::filesystem::exists(session_path, ec) && std::filesystem::file_size(session_path) != 0) {
                aria2c_settings["input-file"] = quote_path(session_path);
            }
            // 检查是否开启全局代理
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
			// 更新当前aria2c 的所有 暂停 正在下载 停止的任务状态列表
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
				if (stopped_num_ > 0 && (active_num_ > 0 || waiting_num_ > 0)) {
					websocket_client_.TellStopped(0, 100, keys);
				}

			} catch (std::exception& e) {
				LOG_ERR("{}", e.what());
			}
		}

		void Aria2cDownloadManager::SyncMagnetServerList() {
			auto start_time = std::chrono::steady_clock::now();

			// 发布开始状态
			pub_sub_system_.Publish(kAria2TrackerUpdateStatus,
			                        R"({"status":"started","message":"Updating tracker list..."})");

			try {
				// 1. 下载黑名单
				const std::string trackers_black_url(
					"https://bitbucket.org/xiu2/trackerslistcollection/raw/master/blacklist.txt");
				auto bt_exclude_tracker = GetBitTorrentUrlWithFallback(trackers_black_url);

				if (!bt_exclude_tracker.empty()) {
					websocket_client_.ChangeGlobalOption({{"bt-exclude-tracker", bt_exclude_tracker}});
					LOG_INFO("Updated tracker blacklist: {} trackers", CountTrackers(bt_exclude_tracker));
				} else {
					LOG_WARN("Failed to fetch tracker blacklist");
				}

				// 2. 获取配置的源列表
				auto json_data = config::GetValue(config::Keys::TrackerSourceNames).AsString();
				nlohmann::json tracker_source_names = nlohmann::json::parse(json_data.c_str());

				if (!tracker_source_names.is_array()) {
					throw std::runtime_error("Invalid tracker_source_names format");
				}

				// 3. 使用 unordered_set 去重
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
						// 拆分并添加到 set（自动去重）
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

				// 4. 转换为逗号分隔字符串
				std::string bt_tracker;
				for (const auto& url : unique_trackers) {
					if (!bt_tracker.empty()) bt_tracker += ",";
					bt_tracker += url;
				}

				auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - start_time);

				if (!bt_tracker.empty()) {
					// 5. 同步到 aria2c
					websocket_client_.ChangeGlobalOption({{"bt-tracker", bt_tracker}});

					// 6. 发布成功通知
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

		void Aria2cDownloadManager::SyncGlobalStatInfo() {
			// Engine 层使用 config 系统
			auto rpc_port = config::GetValue(config::Keys::RpcListenPort).AsString();
			if (rpc_port.empty()) rpc_port = kEngineRpcPort;
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
            // Engine 层使用 config 系统
            auto rpc_port = config::GetValue(config::Keys::RpcListenPort).AsString();
            if (rpc_port.empty()) rpc_port = kEngineRpcPort;
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

		std::string Aria2cDownloadManager::GetBitTorrentUrl(const std::string& url) {
			std::string result;
			try {
				if (url.empty() || !engine_is_runing_) return result;
				auto system_proxy = os::GetSystemHTTPProxy();
				std::string proxy_str;
				if (system_proxy.has_value()) {
					proxy_str =
						"http://" + system_proxy.value().first + ":" + std::to_string(system_proxy.value().second);
				}
				auto reply = cpr::Get(cpr::Url(url), cpr::Proxies({{"http", proxy_str}, {"https", proxy_str}}));
				if (reply.status_code != 200) {
					LOG_ERR("sync manget trackers server list faild error {}", reply.error.message);
					return result;
				}
				result = ParseTextUrls(reply.text);
			} catch (std::exception& e) {
				LOG_ERR("{}", e.what());
			}
			return result;
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

		std::optional<std::string> Aria2cDownloadManager::GetCachedContent(const std::string& url,
																			const std::string& etag) {
			auto entry_opt = cache::TrackerETagCache::Instance().GetEntry(url);
			if (entry_opt && entry_opt->etag == etag) {
				LOG_INFO("Using cached content for: {} (ETag: {})", url, etag);
				return entry_opt->content;
			}
			return std::nullopt;
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
				// 替换域名
				cdn_url.replace(pos, 25, "cdn.jsdelivr.net/gh");

				// 找到第三个 '/' 并替换为 '@'
				// 格式: https://cdn.jsdelivr.net/gh/user/repo/branch/file
				//                                      ^1   ^2  ^3
				size_t start_pos = cdn_url.find("gh/") + 3;
				int slash_count = 0;
				for (size_t i = start_pos; i < cdn_url.size(); ++i) {
					if (cdn_url[i] == '/') {
						slash_count++;
						if (slash_count == 2) {  // user/repo 之后的第一个 '/'
							cdn_url[i] = '@';
							break;
						}
					}
				}
			}

			return cdn_url;
		}

		std::string Aria2cDownloadManager::ConvertToGitHubProxy(const std::string& url) {
			// https://raw.githubusercontent.com/...
			// -> https://gh-proxy.com/https://raw.githubusercontent.com/...
			std::vector<std::string> domains = {"https://ghfast.top/", "https://gh-proxy.com/"};
			// 随机返回一个域名
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<int> dis(0, domains.size() - 1);
			std::string domain = domains[dis(gen)];
			return domain + url;
		}

		std::string Aria2cDownloadManager::GetBitTorrentUrlWithFallback(const std::string& url) {
			if (url.empty() || !engine_is_runing_) return "";

			// 构建降级 URL 列表
			std::vector<std::string> fallback_urls = {url};

			// 如果是 GitHub Raw，添加 CDN 镜像
			if (url.find("raw.githubusercontent.com") != std::string::npos) {
				fallback_urls.push_back(ConvertToJsDelivrCDN(url));
				fallback_urls.push_back(ConvertToGitHubProxy(url));
			}

			// 获取系统代理
			auto system_proxy = os::GetSystemHTTPProxy();
			std::string proxy_str;
			if (system_proxy.has_value()) {
				proxy_str = "http://" + system_proxy.value().first + ":" +
				            std::to_string(system_proxy.value().second);
			}

			// 尝试每个 URL（带重试）
			for (const auto& try_url : fallback_urls) {
				// 从数据库检查是否有缓存的 ETag
				std::string cached_etag;
				auto cached_entry = cache::TrackerETagCache::Instance().GetEntry(try_url);
				if (cached_entry) {
					cached_etag = cached_entry->etag;
					LOG_INFO("Found cached ETag for: {} (ETag: {})", try_url, cached_etag);
				}

				for (int retry = 0; retry < 3; ++retry) {
					try {
						LOG_INFO("Fetching from: {} (attempt {})", try_url, retry + 1);

						// 构建请求头
						cpr::Header headers;
						if (!cached_etag.empty()) {
							headers["If-None-Match"] = cached_etag;
							LOG_INFO("Sending If-None-Match: {}", cached_etag);
						}

						auto reply = cpr::Get(cpr::Url(try_url),
						                      cpr::Proxies({{"http", proxy_str}, {"https", proxy_str}}),
						                      cpr::Timeout(10000),  // 10秒超时
						                      headers);

						// 处理 304 Not Modified - 使用缓存内容
						if (reply.status_code == 304) {
							if (cached_entry) {
								LOG_INFO("HTTP 304 Not Modified, using cached content for: {}", try_url);
								return cached_entry->content;
							} else {
								LOG_WARN("HTTP 304 but no cached content found for: {}", try_url);
								continue;
							}
						}

						// 处理 200 OK - 保存新内容和 ETag
						if (reply.status_code == 200) {
							auto result = ParseTextUrls(reply.text);

							// 提取并保存 ETag
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

					// 重试前等待（指数退避：1s, 2s）
					if (retry < 2) {
						std::this_thread::sleep_for(std::chrono::seconds(1 << retry));
					}
				}
			}

			LOG_ERR("All fallback URLs failed for: {}", url);
			return "";
		}

		Aria2cDownloadManager::~Aria2cDownloadManager() {}

		bool Aria2cDownloadManager::InitAria2cEngine(const String_View& aria2c_path) {
			// todo 初始化aria2c
			aria2c_path_			 = String(aria2c_path);
			std::vector<String> args = InitAria2cSettingsArgs();
			auto pid				 = process::Execute(aria2c_path, args);
			if (pid <= 0) {
				LOG_ERR("Failed to initialise aria2c Failed to start the process");
				return false;
			}

			// 初始化 ETag 缓存数据库
			InitializeETagCache();

			// 启动 更新当前aria2c 的所有 暂停 正在下载 停止的任务状态列表 定时器
			if (config::GetValue(config::Keys::EnableTrackerSourceAutoUpdate).AsBool()) {
				update_aria2c_tasks_timer_.Start(std::bind(&Aria2cDownloadManager::UpdateAria2cTasks, this),
												 std::chrono::milliseconds(300), true);
			}
			// 链接 aria2c websocket
			websocket_client_.Open();
			engine_is_runing_ = true;

			return true;
		}

		void Aria2cDownloadManager::UninitAria2cEngine() {
			// todo 卸载aria2c
			engine_is_runing_ = false;
			update_aria2c_tasks_timer_.Stop();
			daily_task_timer_.Stop();
			websocket_client_.PurgeDownloadResult();
			websocket_client_.Shutdown();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
			// 直接添加到下载引擎任务队列中
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
			boost::asio::post(io_context_, [this]() { SyncMagnetServerList(); });

		}

		void Aria2cDownloadManager::UnSubscribeAria2Message(Subscription subscription) {
			return pub_sub_system_.Unsubscribe(subscription);
		}

	}  // namespace engine

}  // namespace gdl

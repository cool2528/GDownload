#include "aria2c_manager.h"
#include <cpr/cpr.h>
#include <boost/url.hpp>
#include <nlohmann/json.hpp>
#include "engine_def.h"
#include "logger.h"
#include "os/os.h"
#include "process/process.h"
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
			  websocket_client_(std::string("ws://127.0.0.1:") + kEngineRpcPort + "/jsonrpc", io_context_) {

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
			aria2c_settings["rpc-listen-port"]			  = kEngineRpcPort;
			aria2c_settings["rpc-secret"]				  = kDefaultRpcSecret;
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
			// BitTorrent tracker server blacklist
			const std::string trackers_black_url(
				"https://bitbucket.org/xiu2/trackerslistcollection/raw/master/blacklist.txt");
			auto bt_exclude_tracker = GetBitTorrentUrl(trackers_black_url);
			if (!bt_exclude_tracker.empty()) {
				// Set if the obtained blacklist is not empty
				websocket_client_.ChangeGlobalOption({std::make_pair("bt-exclude-tracker", bt_exclude_tracker)});
			}
			else {
				LOG_WARN("If the list of blacklists is empty, then");
			}
			// Get BitTorrent tracker server list from configuration file
			try {
                auto json_data					   = config::GetValue(config::Keys::TrackerSourceUrls).AsString();
				nlohmann::json tracker_source_urls = nlohmann::json::parse(json_data.c_str());
				if (!tracker_source_urls.is_array()) {
					LOG_ERR("Invalid tracker_source_urls {}", json_data);
					return;
				}
				std::string bt_tracker;
				for (const auto& url : tracker_source_urls) {
					if (url.is_string()) {
						std::string source_url = url.get<std::string>();
                        auto bt_tracker_urls   = GetBitTorrentUrl(source_url);
						bt_tracker += "," + bt_tracker_urls;
					}
					if (!engine_is_runing_) break;
				}
				if (!bt_tracker.empty()) {
					// Set if the obtained BitTorrent URL list is not empty
					websocket_client_.ChangeGlobalOption({std::make_pair("bt-tracker", bt_tracker)});
				}

			} catch (std::exception& e) {
				LOG_ERR("Failed to parse tracker_source_urls {}", e.what());
			}
		}

		void Aria2cDownloadManager::SyncGlobalStatInfo() {
			const std::string host = std::string("http://127.0.0.1:") + kEngineRpcPort;
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
            const std::string host					   = std::string("http://127.0.0.1:") + kEngineRpcPort;
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
			// 启动 更新当前aria2c 的所有 暂停 正在下载 停止的任务状态列表 定时器
			update_aria2c_tasks_timer_.Start(std::bind(&Aria2cDownloadManager::UpdateAria2cTasks, this),
											 std::chrono::milliseconds(300), true);
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
			const String& url, const std::unordered_multimap<std::string, std::string>& options) {
			std::string real_url										   = url;
			std::unordered_multimap<std::string, std::string> real_options = options;
			// 直接添加到下载引擎任务队列中
			return websocket_client_.AddUri({real_url}, real_options);
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

		void Aria2cDownloadManager::UnSubscribeAria2Message(Subscription subscription) {
			return pub_sub_system_.Unsubscribe(subscription);
		}

	}  // namespace engine

}  // namespace gdl

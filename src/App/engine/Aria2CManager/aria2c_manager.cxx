#include "aria2c_manager.h"
#include "engine_def.h"
#include "logger.h"
#include "os/os.h"
#include "process/process.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "config/config.h"
namespace gdl {
	namespace engine {

		namespace detail {}
		Aria2cDownloadManager::Aria2cDownloadManager()
			: work_(boost::asio::make_work_guard(io_context_)),
			  daily_task_timer_(io_context_),
			  update_aria2c_tasks_timer_(io_context_) {
			worker_ = std::thread([this] { io_context_.run(); });
			daily_task_timer_.Start([this] {
				// 更新 磁力链接 每日列表
			});
		}

		std::vector<String_View> Aria2cDownloadManager::InitAria2cSettingsArgs() {
			std::vector<String_View> result;
			std::unordered_map<std::string, std::string> aria2c_settings;
			aria2c_settings["allow-overwrite"]		  = "false";
			aria2c_settings["auto-file-renaming"]	  = "true";
			aria2c_settings["bt-exclude-tracker"]	  = "";
			aria2c_settings["bt-force-encryption"]	  = "false";
			aria2c_settings["bt-load-saved-metadata"] = "true";
			aria2c_settings["bt-save-metadata"]		  = "true";
			aria2c_settings["bt-tracker"]			  = "";
			aria2c_settings["continue"]				  = "true";
			aria2c_settings["dht-file-path"]		  = GetDhtPath(IP_VERSION::V4);
			aria2c_settings["dht-file-path6"]		  = GetDhtPath(IP_VERSION::V6);

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
			aria2c_settings["user-agent"] =
				"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) "
				"Chrome/111.0.0.0 Safari/537.36";
			aria2c_settings["all-proxy"]		 = "";
			aria2c_settings["check-certificate"] = "false";
			aria2c_settings["quiet"]			 = "true";
			aria2c_settings["enable-rpc"]		 = "true";
			aria2c_settings["rpc-listen-all"]	 = "true";
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
			return os::GetAppDataDir() + "/" + name;
		}

		void Aria2cDownloadManager::UpdateAria2cTasks() {
			// 更新当前aria2c 的所有 暂停 正在下载 停止的任务状态列表
		}

		Aria2cDownloadManager::~Aria2cDownloadManager() {
			UninitAria2cEngine();
			work_.reset();
			io_context_.stop();
			worker_.join();
		}

		bool Aria2cDownloadManager::InitAria2cEngine(const String_View& aria2c_path) {
			// todo 初始化aria2c
			aria2c_path_				  = String(aria2c_path);
			std::vector<String_View> args = InitAria2cSettingsArgs();
			auto pid					  = process::Execute(aria2c_path, args);
			if (pid >= 0) {
				LOG_ERR("Failed to initialise aria2c Failed to start the process");
				return false;
			}
			engine_is_runing_ = true;
			// 启动 更新当前aria2c 的所有 暂停 正在下载 停止的任务状态列表 定时器
			update_aria2c_tasks_timer_.Start(std::bind(&Aria2cDownloadManager::UpdateAria2cTasks, this),
											 std::chrono::seconds(2), true);
			return true;
		}

		void Aria2cDownloadManager::UninitAria2cEngine() {
			// todo 卸载aria2c
			engine_is_runing_ = false;
			daily_task_timer_.Stop();
		}

	}  // namespace engine

}  // namespace gdl

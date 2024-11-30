#pragma once
#include <atomic>
#include <boost/asio.hpp>
#include "Engine_export.h"
#include "aria2c_websocket_rpc_client.h"
#include "globalTypes.h"
#include "publish_subscribe_system.h"
#include "result/result.h"
#include "singleton.hpp"
#include "timer/asio_async_timer.h"
#include "timer/daily_task_timer.h"
namespace gdl {
	namespace engine {
		using Subscription = std::shared_ptr<PubSubSystem<std::string>::Subscription>;
		enum class IP_VERSION : int { V4 = 4, V6 = 6 };
		class Engine_API Aria2cDownloadManager : public Singleton<Aria2cDownloadManager> {
			SINGLETON_DECLARE(Aria2cDownloadManager)
		   public:
			~Aria2cDownloadManager();
			bool InitAria2cEngine(const String_View& aria2c_path);
			void UninitAria2cEngine();
			bool EngineIsRuning() const { return engine_is_runing_; }
			Result<bool> AddHttpTask(const String& url,
									 const std::unordered_multimap<std::string, std::string>& options);

			Result<bool> AddTorrentTask(const String& tarrent,
										const std::unordered_multimap<std::string, std::string>& options);

			Result<bool> AddMetalinkTask(const String& metalink,
										 const std::unordered_multimap<std::string, std::string>& options);

			Result<Subscription> SubscriptionAria2Message(const std::string& topic,
														  std::function<void(const std::string&)> handler);

			void UnSubscribeAria2Message(Subscription subscription);

		   private:
			explicit Aria2cDownloadManager();
			std::vector<String> InitAria2cSettingsArgs();
			String GetDhtPath(IP_VERSION protocol);
			void UpdateAria2cTasks();
			void SyncMagnetServerList();
			std::string ParseTextUrls(const std::string& input);
			std::string GetBitTorrentUrl(const std::string& url);

		   private:
			String aria2c_path_;
			boost::asio::io_context io_context_;
			std::atomic_bool engine_is_runing_{false};
			std::atomic_bool daily_task_timer_is_runing{false};
			boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;
			DailyTaskTimer daily_task_timer_;
			AsyncTimer update_aria2c_tasks_timer_;
			std::thread worker_;
			PubSubSystem<std::string> pub_sub_system_;
			Aria2cWebSocketClient websocket_client_;
		};
	}  // namespace engine

}  // namespace gdl

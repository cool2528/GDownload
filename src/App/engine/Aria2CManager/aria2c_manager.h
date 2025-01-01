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
		enum class Aria2Method : int{
			kAddUri,
			kAddTorrent,
			kAddMetalink,
			kRemove,
			kForceRemove,
			kPause,
			kPauseAll,
			kForcePause,
			kForcePauseAll,
			kUnpause,
			kUnpauseAll,
			kTellStatus,
			kGetUris,
			kGetFiles,
			kGetPeers,
			kGetServers,
			kTellActive,
			kTellWaiting,
			kTellStopped,
			kChangePosition,
			kGetOption,
			kChangeOption,
			kGetGlobalOption,
			kChangeGlobalOption,
			kGetGlobalStat,
			kPurgeDownloadResult,
			kRemoveDownloadResult,
			kGetVersion
		};

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

			template<typename ...Args>
			Result<bool> CallAria2cMethod(const Aria2Method& method, Args&&... args) {
				switch (method) {
					case Aria2Method::kAddUri:
						return AddHttpTask(std::forward<Args>(args)...);
					case Aria2Method::kAddTorrent:
						return AddTorrentTask(std::forward<Args>(args)...);
					case Aria2Method::kAddMetalink:
						return AddMetalinkTask(std::forward<Args>(args)...);
					case Aria2Method::kRemove:
						return websocket_client_.Remove(std::forward<Args>(args)...);
					case Aria2Method::kForceRemove:
						return websocket_client_.ForceRemove(std::forward<Args>(args)...);
					case Aria2Method::kPause:
						return websocket_client_.Pause(std::forward<Args>(args)...);
					case Aria2Method::kPauseAll:
						return websocket_client_.PauseAll();
					case Aria2Method::kForcePause:
						return websocket_client_.ForcePause(std::forward<Args>(args)...);
					case Aria2Method::kForcePauseAll:
						return websocket_client_.ForcePauseAll();
					case Aria2Method::kUnpause:
						return websocket_client_.Unpause(std::forward<Args>(args)...);
					case Aria2Method::kUnpauseAll:
						return websocket_client_.UnpauseAll();
					case Aria2Method::kTellStatus:
						return websocket_client_.TellStatus(std::forward<Args>(args)...);
					case Aria2Method::kGetUris:
						return websocket_client_.GetUris(std::forward<Args>(args)...);
					case Aria2Method::kGetFiles:
						return websocket_client_.GetFiles(std::forward<Args>(args)...);
					case Aria2Method::kGetPeers:
						return websocket_client_.GetPeers(std::forward<Args>(args)...);
					case Aria2Method::kGetServers:
						return websocket_client_.GetServers(std::forward<Args>(args)...);
					case Aria2Method::kTellActive:
						return websocket_client_.TellActive(std::forward<Args>(args)...);
					case Aria2Method::kTellWaiting:
						return websocket_client_.TellWaiting(std::forward<Args>(args)...);
					case Aria2Method::kTellStopped:
						return websocket_client_.TellStopped(std::forward<Args>(args)...);
					case Aria2Method::kChangePosition:
						return websocket_client_.ChangePosition(std::forward<Args>(args)...);
					case Aria2Method::kGetOption:
						return websocket_client_.GetOption(std::forward<Args>(args)...);
					case Aria2Method::kChangeOption:
						return websocket_client_.changeOption(std::forward<Args>(args)...);
					case Aria2Method::kGetGlobalOption:
						return websocket_client_.GetGlobalOption();
					case Aria2Method::kChangeGlobalOption:
						return websocket_client_.ChangeGlobalOption(std::forward<Args>(args)...);
					case Aria2Method::kGetGlobalStat:
						return websocket_client_.GetGlobalStat();
					case Aria2Method::kPurgeDownloadResult:
						return websocket_client_.PurgeDownloadResult(std::forward<Args>(args)...);
					case Aria2Method::kRemoveDownloadResult:
						return websocket_client_.RemoveDownloadResult(std::forward<Args>(args)...);
					case Aria2Method::kGetVersion:
						return websocket_client_.GetVersion();
					default:
						break;
				}
				return MakeFail(1,"Invalid methodology request");
			}

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
			std::vector<std::thread> worker_threads_;
			PubSubSystem<std::string> pub_sub_system_;
			Aria2cWebSocketClient websocket_client_;
		};
	}  // namespace engine

}  // namespace gdl

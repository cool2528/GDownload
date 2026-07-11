#pragma once
#include <atomic>
#include <future>
#include "tracker_sync_gate.h"
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
        enum class Aria2Method : int {
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
			kGetVersion,
			kShutdown,
            kForceShutdown,
            kMulticall
		};

		class Engine_API Aria2cDownloadManager : public Singleton<Aria2cDownloadManager> {
			SINGLETON_DECLARE(Aria2cDownloadManager)
		   public:
			~Aria2cDownloadManager();
			bool InitAria2cEngine(const String_View& aria2c_path);
			void UninitAria2cEngine();
			bool EngineIsRuning() const { return engine_is_runing_; }
            Result<bool> AddHttpTask(const std::vector<String>& url,
									 const std::unordered_multimap<std::string, std::string>& options);

			Result<bool> AddTorrentTask(const String& tarrent,
										const std::unordered_multimap<std::string, std::string>& options);

			Result<bool> AddMetalinkTask(const String& metalink,
										 const std::unordered_multimap<std::string, std::string>& options);

			Result<Subscription> SubscriptionAria2Message(const std::string& topic,
														  std::function<void(const std::string&)> handler);
			void UpdateMagnetServerList();
            template <typename... Args>
			Result<bool> CallAria2cMethod(const Aria2Method& method, Args&&... args) {
				using TupleType = std::tuple<Args...>;
				if constexpr (sizeof...(Args) == 0) {
					switch (method) {
						case Aria2Method::kPauseAll:
							return websocket_client_.PauseAll();
						case Aria2Method::kForcePauseAll:
							return websocket_client_.ForcePauseAll();
						case Aria2Method::kUnpauseAll:
							return websocket_client_.UnpauseAll();
						case Aria2Method::kGetGlobalStat:
							return websocket_client_.GetGlobalStat();
						case Aria2Method::kPurgeDownloadResult:
							return websocket_client_.PurgeDownloadResult();
						case Aria2Method::kGetVersion:
							return websocket_client_.GetVersion();
						case Aria2Method::kShutdown:
							return websocket_client_.Shutdown();
						case Aria2Method::kForceShutdown:
							return websocket_client_.ForceShutdown();
						case Aria2Method::kGetGlobalOption:
							return websocket_client_.GetGlobalOption();
						default:
							return MakeFail(1, "Invalid method or arguments");
					}
				}
                else if constexpr (sizeof...(Args) == 1) {
                    using FirstType = std::decay_t<std::tuple_element_t<0, TupleType>>;
					if constexpr (std::is_same_v<FirstType, std::string>) {
						switch (method) {
							case Aria2Method::kRemove:
								return websocket_client_.Remove(std::forward<Args>(args)...);
							case Aria2Method::kForceRemove:
								return websocket_client_.ForceRemove(std::forward<Args>(args)...);
							case Aria2Method::kPause:
								return websocket_client_.Pause(std::forward<Args>(args)...);
							case Aria2Method::kForcePause:
								return websocket_client_.ForcePause(std::forward<Args>(args)...);
							case Aria2Method::kUnpause:
								return websocket_client_.Unpause(std::forward<Args>(args)...);
							case Aria2Method::kGetUris:
								return websocket_client_.GetUris(std::forward<Args>(args)...);
							case Aria2Method::kGetFiles:
								return websocket_client_.GetFiles(std::forward<Args>(args)...);
							case Aria2Method::kGetPeers:
								return websocket_client_.GetPeers(std::forward<Args>(args)...);
							case Aria2Method::kGetServers:
								return websocket_client_.GetServers(std::forward<Args>(args)...);
							case Aria2Method::kRemoveDownloadResult:
								return websocket_client_.RemoveDownloadResult(std::forward<Args>(args)...);
							default:
								return MakeFail(1, "Invalid method or arguments");
						}
					}
                    else if constexpr (std::is_same_v<FirstType, std::vector<std::string>>) {
						switch (method) {
							case Aria2Method::kTellActive:
								return websocket_client_.TellActive(std::forward<Args>(args)...);
							default:
								return MakeFail(1, "Invalid method or arguments");
						}
					}
					else if constexpr (std::is_same_v<FirstType, Options>) {
						switch (method) {
							case Aria2Method::kChangeGlobalOption:
								return websocket_client_.ChangeGlobalOption(std::forward<Args>(args)...);
                            case Aria2Method::kMulticall:
                                return websocket_client_.Multicall(std::forward<Args>(args)...);
							default:
								return MakeFail(1, "Invalid method or arguments");
						}
					}
					return MakeFail(1, "Invalid method or arguments");
				}
                else if constexpr (sizeof...(Args) == 2) {
                    using FirstType	 = std::decay_t<std::tuple_element_t<0, TupleType>>;
					using SecondType = std::decay_t<std::tuple_element_t<1, TupleType>>;
					if constexpr (std::is_same_v<FirstType, std::string> && std::is_same_v<SecondType, Options>) {
						switch (method) {
							case Aria2Method::kAddTorrent:
								return AddTorrentTask(std::forward<Args>(args)...);
							case Aria2Method::kAddMetalink:
								return AddMetalinkTask(std::forward<Args>(args)...);
							case Aria2Method::kChangeOption:
								return websocket_client_.changeOption(std::forward<Args>(args)...);
							default:
								return MakeFail(1, "Invalid method or arguments");
						}
					}
                    else if constexpr (std::is_same_v<FirstType, std::string> &&
                                       std::is_same_v<SecondType, std::vector<std::string>>) {
						switch (method) {
							case Aria2Method::kTellStatus:
								return websocket_client_.TellStatus(std::forward<Args>(args)...);
							default:
								return MakeFail(1, "Invalid method or arguments");
						}
                    }
                    else if constexpr (std::is_same_v<FirstType, std::vector<std::string>> &&
                                       std::is_same_v<SecondType, Options>) {
                        switch (method) {
                            case Aria2Method::kAddUri:
                                return AddHttpTask(std::forward<Args>(args)...);
                            default:
                                return MakeFail(1, "Invalid method or arguments");
                        }
                    }
				}
				else if constexpr (sizeof...(Args) == 3) {
					using FirstType	 = std::decay_t<std::tuple_element_t<0, TupleType>>;
					using SecondType = std::decay_t<std::tuple_element_t<1, TupleType>>;
					using ThirdType	 = std::decay_t<std::tuple_element_t<2, TupleType>>;
					if constexpr (std::is_same_v<FirstType, std::string> && std::is_same_v<SecondType, int> &&
								  std::is_same_v<ThirdType, int>) {
						switch (method) {
							case Aria2Method::kChangePosition:
								return websocket_client_.ChangePosition(std::forward<Args>(args)...);
							default:
								return MakeFail(1, "Invalid method or arguments");
						}
					}
					else if constexpr (std::is_same_v<FirstType, int> && std::is_same_v<SecondType, int> &&
									   std::is_same_v<ThirdType, std::vector<std::string>>) {
						switch (method) {
							case Aria2Method::kTellWaiting:
								return websocket_client_.TellWaiting(std::forward<Args>(args)...);
							case Aria2Method::kTellStopped:
								return websocket_client_.TellStopped(std::forward<Args>(args)...);
							default:
								return MakeFail(1, "Invalid method or arguments");
						}
					}
				}
				return MakeFail(1, "Invalid method or arguments");
			}

			void UnSubscribeAria2Message(Subscription subscription);

		   private:
			explicit Aria2cDownloadManager();
			std::vector<String> InitAria2cSettingsArgs();
			String GetDhtPath(IP_VERSION protocol);
			void UpdateAria2cTasks();
			void SyncMagnetServerList();
			void DispatchMagnetServerSync();
			void SyncGlobalStatInfo();
            std::string SyncActiveTaskInfo();
			std::string ParseTextUrls(const std::string& input);
			// Tracker 优化相关方法
			/// 带降级策略的 URL 获取
			std::string GetBitTorrentUrlWithFallback(const std::string& url);
			/// GitHub Raw 转 jsDelivr CDN
			std::string ConvertToJsDelivrCDN(const std::string& url);
			/// 统计 Tracker 数量
			int CountTrackers(const std::string& tracker_list);

			// ETag 缓存相关方法（使用数据库存储）
			/// 初始化 ETag 缓存数据库
			void InitializeETagCache();
			/// 更新缓存条目
			void UpdateCacheEntry(const std::string& url, const std::string& etag, const std::string& content);



		   private:
			String aria2c_path_;
			boost::asio::io_context io_context_;
			std::atomic_bool engine_is_runing_{false};
			std::atomic_bool uninited_{false};
			std::atomic_bool daily_task_timer_is_runing{false};
			boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;
			DailyTaskTimer daily_task_timer_;
			AsyncTimer update_aria2c_tasks_timer_;
			std::vector<std::thread> worker_threads_;
			PubSubSystem<std::string> pub_sub_system_;
			Aria2cWebSocketClient websocket_client_;
			std::atomic_int64_t active_num_{0};
			std::atomic_int64_t waiting_num_{0};
			std::atomic_int64_t stopped_num_{0};
			// tracker 同步移出 io 线程（E3）：独立线程执行阻塞 HTTP，Uninit 时 wait
			TrackerSyncGate tracker_sync_gate_;
			std::int64_t aria2c_pid_{-1};  // aria2c 子进程 pid，退出时优雅关闭并回收（S3）
		};
	}  // namespace engine

}  // namespace gdl

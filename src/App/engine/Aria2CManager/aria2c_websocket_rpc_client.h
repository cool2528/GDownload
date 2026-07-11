#pragma once
#include <rapidjson/document.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include "Engine_export.h"
#include "result/result.h"
#include "websocket_client.h"
namespace gdl {
	namespace engine {
		using Options = std::unordered_multimap<std::string, std::string>;
		enum class State : int { kError = -1, kConnected, kClosed };
		class Engine_API Aria2cWebSocketClient {

		   public:
			class Engine_API SynchronizedStateCallback {
			   public:
				using Callback = std::function<void(const State&, std::string)>;
				void Set(Callback callback);
				void Clear();
				void Invoke(const State& state, std::string message);

			   private:
				std::mutex mutex_;
				std::condition_variable idle_;
				Callback callback_;
				size_t active_callbacks_{0};
			};

            explicit Aria2cWebSocketClient(const std::string& url, boost::asio::io_context& ioc);
			~Aria2cWebSocketClient();

		   public:
			void Open();
			void Disconnect();
			bool IsConnected() const;
			void DisableAutoReconnect();
			Result<bool> AddUri(const std::vector<std::string>& uris, const Options& options);
			Result<bool> AddTorrent(const std::string& torrent, const Options& options);
			Result<bool> AddMetalink(const std::string& metalink, const Options& options);
			Result<bool> Remove(const std::string& gid);
			Result<bool> ForceRemove(const std::string& gid);
			Result<bool> Pause(const std::string& gid);
			Result<bool> PauseAll();
			Result<bool> ForcePause(const std::string& gid);
			Result<bool> ForcePauseAll();
			Result<bool> Unpause(const std::string& gid);
			Result<bool> UnpauseAll();
			Result<bool> TellStatus(const std::string& gid, const std::vector<std::string>& keys);
			//getUris
			Result<bool> GetUris(const std::string& gid);
			//getFiles
			Result<bool> GetFiles(const std::string& gid);
			//getPeers
			Result<bool> GetPeers(const std::string& gid);
			// getServers
			Result<bool> GetServers(const std::string& gid);
			//tellActive
			Result<bool> TellActive(const std::vector<std::string>& keys);
			//tellWaiting
			Result<bool> TellWaiting(int offset, int num, const std::vector<std::string>& keys);
			//tellStopped
			Result<bool> TellStopped(int offset, int num, const std::vector<std::string>& keys);
			//changePosition
			Result<bool> ChangePosition(const std::string& gid, int pos, int how);
			//getOption
			Result<bool> GetOption(const std::string& gid);
			//changeOption
			Result<bool> changeOption(const std::string& gid, const Options& options);
			//getGlobalOption
			Result<bool> GetGlobalOption();
			//changeGlobalOption
			Result<bool> ChangeGlobalOption(const Options& options);
			//getGlobalStat
			Result<bool> GetGlobalStat();
			//purgeDownloadResult
			Result<bool> PurgeDownloadResult();
			//removeDownloadResult
			Result<bool> RemoveDownloadResult(const std::string& gid);
			//getVersion
			Result<bool> GetVersion();
			//shutdown
			Result<bool> Shutdown();
			//forceShutdown
			Result<bool> ForceShutdown();
            // multicall
            Result<bool> Multicall(const Options& methods);

		   public:
			void SetMessageCallback(const std::function<void(const std::string&)>& cb);
			void SetStateChanageCallback(const std::function<void(const State&, std::string)>&);
			void ClearStateChanageCallback();

		   private:
			void onConnected();
			void onClosed();
			void onTextMessageReceived(std::string message);

		   private:
            Result<bool> Send(const std::string_view& method, rapidjson::Value& params);

		   private:
			std::string url_;
			std::shared_ptr<WebSocketClient> websocket_;
			// 注：原共享成员 doc_ 已移除，改为各 RPC 方法内使用局部 rapidjson::Document，
			// 避免多线程并发调用时对同一 Document 的数据竞争。

			struct CallbackState {
				std::mutex mutex;
				std::atomic<bool> alive{true};
				std::function<void(const std::string&)> text_message_callback{nullptr};
			};
			std::shared_ptr<CallbackState> callback_state_;
			std::shared_ptr<SynchronizedStateCallback> state_callback_;
		};
	}  // namespace engine
}  // namespace gdl

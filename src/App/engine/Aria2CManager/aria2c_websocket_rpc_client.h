#pragma once
#include <QWebSocket>
#include <nlohmann/json.hpp>
#include "result/result.h"
namespace gdl {
	namespace engine {
		using Options = std::unordered_multimap<std::string, std::string>;
		class Aria2cWebSocketClient : public QObject {
			Q_OBJECT
		   public:
			explicit Aria2cWebSocketClient(const QString& url, QObject* parent = nullptr);
			~Aria2cWebSocketClient() override;

		   public:
			void Open();
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

		   public:
			Q_SIGNAL void MessageReceived(QString message);
			Q_SIGNAL void StateChanged(QAbstractSocket::SocketState state);

		   private Q_SLOTS:

			void onConnected();
			void onClosed();
			void onTextMessageReceived(QString message);

		   private:
			Result<bool> Send(const std::string_view& method, const nlohmann::json& params);

		   private:
			QString url_;
			QWebSocket websocket_;
		};
	}  // namespace engine
}  // namespace gdl

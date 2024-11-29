#include "aria2c_websocket_rpc_client.h"
#include "engine_def.h"
#include "logger.h"
namespace gdl {
	namespace engine {

		Aria2cWebSocketClient::Aria2cWebSocketClient(const QString& url, QObject* parent) : url_(url), QObject(parent) {
			connect(&websocket_, &QWebSocket::connected, this, &Aria2cWebSocketClient::onConnected);
			connect(&websocket_, &QWebSocket::disconnected, this, &Aria2cWebSocketClient::onClosed);
			connect(&websocket_, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) {
				int err_code = static_cast<int>(error);
				LOG_ERR("connect websocket faild error code  {}", err_code);
			});
			connect(&websocket_, &QWebSocket::textMessageReceived, this, &Aria2cWebSocketClient::onTextMessageReceived);
			connect(&websocket_, &QWebSocket::binaryMessageReceived, this,
					&Aria2cWebSocketClient::onTextMessageReceived);
			connect(&websocket_, &QWebSocket::stateChanged, this, &Aria2cWebSocketClient::StateChanged);
		}

		Aria2cWebSocketClient::~Aria2cWebSocketClient() {
			websocket_.close();
		}

		void Aria2cWebSocketClient::Open() {
			websocket_.open(url_);
		}

		Result<bool> Aria2cWebSocketClient::AddUri(const std::vector<std::string>& uris, const Options& options) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(uris);
			params.push_back(options);
			return Send("aria2.addUri", params);
		}

		Result<bool> Aria2cWebSocketClient::AddTorrent(const std::string& torrent, const Options& options) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(torrent);
			params.push_back(options);
			return Send("aria2.addTorrent", params);
		}

		Result<bool> Aria2cWebSocketClient::AddMetalink(const std::string& metalink, const Options& options) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(metalink);
			params.push_back(options);
			return Send("aria2.addMetalink", params);
		}

		Result<bool> Aria2cWebSocketClient::Remove(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.remove", params);
		}

		Result<bool> Aria2cWebSocketClient::ForceRemove(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.forceRemove", params);
		}

		Result<bool> Aria2cWebSocketClient::Pause(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.pause", params);
		}

		Result<bool> Aria2cWebSocketClient::PauseAll() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.pauseAll", params);
		}

		Result<bool> Aria2cWebSocketClient::ForcePause(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.forcePause", params);
		}

		Result<bool> Aria2cWebSocketClient::ForcePauseAll() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.forcePauseAll", params);
		}

		Result<bool> Aria2cWebSocketClient::Unpause(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.unpause", params);
		}

		Result<bool> Aria2cWebSocketClient::UnpauseAll() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.unpauseAll", params);
		}

		Result<bool> Aria2cWebSocketClient::TellStatus(const std::string& gid, const std::vector<std::string>& keys) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			params.push_back(keys);
			return Send("aria2.tellStatus", params);
		}

		Result<bool> Aria2cWebSocketClient::GetUris(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.getUris", params);
		}

		Result<bool> Aria2cWebSocketClient::GetFiles(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.getFiles", params);
		}

		Result<bool> Aria2cWebSocketClient::GetPeers(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.getPeers", params);
		}

		Result<bool> Aria2cWebSocketClient::GetServers(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.getServers", params);
		}

		Result<bool> Aria2cWebSocketClient::TellActive(const std::vector<std::string>& keys) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(keys);
			return Send("aria2.tellActive", params);
		}

		Result<bool> Aria2cWebSocketClient::TellWaiting(int offset, int num, const std::vector<std::string>& keys) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(offset);
			params.push_back(num);
			params.push_back(keys);
			return Send("aria2.tellWaiting", params);
		}

		Result<bool> Aria2cWebSocketClient::TellStopped(int offset, int num, const std::vector<std::string>& keys) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(offset);
			params.push_back(num);
			params.push_back(keys);
			return Send("aria2.tellStopped", params);
		}

		Result<bool> Aria2cWebSocketClient::ChangePosition(const std::string& gid, int pos, int how) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			params.push_back(pos);
			params.push_back(how);
			return Send("aria2.changePosition", params);
		}

		Result<bool> Aria2cWebSocketClient::GetOption(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.getOption", params);
		}

		Result<bool> Aria2cWebSocketClient::changeOption(const std::string& gid, const Options& options) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			params.push_back(options);
			return Send("aria2.changeOption", params);
		}

		Result<bool> Aria2cWebSocketClient::GetGlobalOption() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.getGlobalOption", params);
		}

		Result<bool> Aria2cWebSocketClient::ChangeGlobalOption(const Options& options) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(options);
			return Send("aria2.changeGlobalOption", params);
		}

		Result<bool> Aria2cWebSocketClient::GetGlobalStat() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.getGlobalStat", params);
		}

		Result<bool> Aria2cWebSocketClient::PurgeDownloadResult() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.purgeDownloadResult", params);
		}

		Result<bool> Aria2cWebSocketClient::RemoveDownloadResult(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.removeDownloadResult", params);
		}

		Result<bool> Aria2cWebSocketClient::GetVersion() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.getVersion", params);
		}

		Result<bool> Aria2cWebSocketClient::Shutdown() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.shutdown", params);
		}

		Result<bool> Aria2cWebSocketClient::ForceShutdown() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.forceShutdown", params);
		}

		void Aria2cWebSocketClient::onConnected() {
			if (websocket_.state() == QAbstractSocket::ConnectedState) {
				GetVersion();
			}
		}

		void Aria2cWebSocketClient::onClosed() {}

		void Aria2cWebSocketClient::onTextMessageReceived(QString message) {
			LOG_DBG("Received websocket msg {}", message.toStdString());
			Q_EMIT MessageReceived(std::move(message));
		}

		Result<bool> Aria2cWebSocketClient::Send(const std::string_view& method, const nlohmann::json& params) {
			nlohmann::json doc;
			static int id = 0;

			doc["jsonrpc"] = "2.0";
			doc["method"]  = method;
			doc["params"]  = params;
			doc["id"]	   = std::to_string(++id);
			if (websocket_.state() != QAbstractSocket::ConnectedState) {
				return MakeFail(static_cast<std::int64_t>(gdl::ErrorType::kUnknownError));
			}
			const auto data = QString::fromStdString(doc.dump());
			const auto len	= websocket_.sendTextMessage(data);
			return len == data.size();
		}

	}  // namespace engine
}  // namespace gdl

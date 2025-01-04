#include "aria2c_websocket_rpc_client.h"
#include <boost/url.hpp>
#include "engine_def.h"
#include "logger.h"
#include "websocket_client.h"
namespace gdl {
	namespace engine {

		Aria2cWebSocketClient::Aria2cWebSocketClient(const std::string& url) : url_(url) {
			websocket_ = std::make_unique<WebSocketClient>();
			// connected
			websocket_->setConnectCallback([this] {
				if (state_chanage_callback_) {
					state_chanage_callback_(State::kConnected, "");
					GetVersion();
				}
			});
			// closed
			websocket_->setDisconnectCallback([this] {
				if (state_chanage_callback_) {
					state_chanage_callback_(State::kClosed, "");
				}
			});
			// error message
			websocket_->setErrorCallback([this](const std::string& error) {
				if (state_chanage_callback_) {
					state_chanage_callback_(State::kClosed, error);
				}
			});
			// receive message
			websocket_->setMessageCallback([this](const std::string& msg) {
				if (text_message_callback_) {
					text_message_callback_(msg);
				}
			});
		}

		Aria2cWebSocketClient::~Aria2cWebSocketClient() {
			websocket_->disconnect();
		}

		void Aria2cWebSocketClient::Open() {
			boost::urls::url ws_server_url(url_);
			auto host	   = ws_server_url.host();
			auto port	   = ws_server_url.port();
			auto path	   = ws_server_url.path();
			const auto res = websocket_->connect(host, std::stol(port), path);
			if (!res) {
				LOG_ERR("connect websocket server faild {}", url_);
			}
		}

		void Aria2cWebSocketClient::Disconnect() {
			websocket_->disconnect();
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

        Result<bool> Aria2cWebSocketClient::Multicall(const Options& methods) {
            try {
                nlohmann::json params = nlohmann::json::array();
                params.push_back(std::format("token:{}", kDefaultRpcSecret));
                for (const auto& method : methods) {
                    nlohmann::json method_object;
                    std::string sub_method_name, sub_method_param;
                    sub_method_name				= method.first;
                    sub_method_param			= method.second;
                    nlohmann::json sub_params	= nlohmann::json::parse(sub_method_param);
                    method_object["methodName"] = sub_method_name;
                    method_object["params"]		= sub_params;
                    params.push_back(method_object);
                }
                return Send("system.multicall", params);
            } catch (...) {}
            return false;
        }

		void Aria2cWebSocketClient::SetMessageCallback(const std::function<void(const std::string&)>& cb) {
			text_message_callback_ = cb;
		}

		void Aria2cWebSocketClient::SetStateChanageCallback(const std::function<void(const State&, std::string)>& cb) {
			state_chanage_callback_ = cb;
		}

		Result<bool> Aria2cWebSocketClient::Send(const std::string_view& method, const nlohmann::json& params) {
			nlohmann::json doc;
			static int id = 0;

			doc["jsonrpc"] = "2.0";
			doc["method"]  = method;
			doc["params"]  = params;
			doc["id"]	   = std::to_string(++id);
			if (!websocket_->isConnect()) {
				return MakeFail(static_cast<std::int64_t>(gdl::ErrorType::kUnknownError));
			}
			const auto data = doc.dump();
			return websocket_->send(data);
		}

	}  // namespace engine
}  // namespace gdl

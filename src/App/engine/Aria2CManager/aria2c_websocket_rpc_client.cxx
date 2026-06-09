#include "aria2c_websocket_rpc_client.h"
#include <format>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <boost/url.hpp>
#include "config/config.h"
#include "engine_def.h"
#include "logger.h"
#include "websocket_client.h"
namespace gdl {
	namespace engine {
		namespace {
			std::string BuildRpcToken() {
				auto rpc_secret = config::GetValue(config::Keys::RpcSecret).AsString();
				if (rpc_secret.empty()) {
					rpc_secret = kDefaultRpcSecret;
				}
				return std::format("token:{}", rpc_secret);
			}
		}  // namespace

		Aria2cWebSocketClient::Aria2cWebSocketClient(const std::string& url, boost::asio::io_context& ioc) : url_(url) {
			websocket_ = std::make_shared<WebSocketClient>(ioc);
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
			auto host = ws_server_url.host();
			auto port = ws_server_url.port();
			auto path = ws_server_url.path();
			websocket_->setAutoReconnect(true, -1, 2);
			const auto res = websocket_->connect(host, port, path);
			if (!res) {
				LOG_ERR("connect websocket server faild {}", url_);
			}
		}

		void Aria2cWebSocketClient::Disconnect() {
			websocket_->disconnect();
		}

		Result<bool> Aria2cWebSocketClient::AddUri(const std::vector<std::string>& uris, const Options& options) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(uris);
			// params.push_back(options);
			// return Send("aria2.addUri", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value uriList(rapidjson::kArrayType);
			for (const auto& uri : uris) {
				rapidjson::Value uriValue;
				uriValue.SetString(uri.c_str(), uri.size(), doc_.GetAllocator());
				uriList.PushBack(uriValue, doc_.GetAllocator());
			}
			params.PushBack(uriList, doc_.GetAllocator());
			rapidjson::Value optionsValue;
			optionsValue.SetObject();
			for (const auto& [key, value] : options) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				rapidjson::Value keyValue;
				keyValue.SetString(value.c_str(), value.size(), doc_.GetAllocator());
				optionsValue.AddMember(keyName, keyValue, doc_.GetAllocator());
			}
			params.PushBack(optionsValue, doc_.GetAllocator());
			return Send("aria2.addUri", params);
		}

		Result<bool> Aria2cWebSocketClient::AddTorrent(const std::string& torrent, const Options& options) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(torrent);
			//          params.push_back(nlohmann::json::array());
			// params.push_back(options);
			// return Send("aria2.addTorrent", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value torrentValue;
			torrentValue.SetString(torrent.c_str(), torrent.size(), doc_.GetAllocator());
			params.PushBack(torrentValue, doc_.GetAllocator());
			rapidjson::Value emptyArray(rapidjson::kArrayType);
			params.PushBack(emptyArray, doc_.GetAllocator());
			rapidjson::Value optionsValue;
			optionsValue.SetObject();
			for (const auto& [key, value] : options) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				rapidjson::Value keyValue;
				keyValue.SetString(value.c_str(), value.size(), doc_.GetAllocator());
				optionsValue.AddMember(keyName, keyValue, doc_.GetAllocator());
			}
			params.PushBack(optionsValue, doc_.GetAllocator());
			return Send("aria2.addTorrent", params);
		}

		Result<bool> Aria2cWebSocketClient::AddMetalink(const std::string& metalink, const Options& options) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(metalink);
			// params.push_back(options);
			// return Send("aria2.addMetalink", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value metalinkValue;
			metalinkValue.SetString(metalink.c_str(), metalink.size(), doc_.GetAllocator());
			params.PushBack(metalinkValue, doc_.GetAllocator());
			rapidjson::Value optionsValue;
			optionsValue.SetObject();
			for (const auto& [key, value] : options) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				rapidjson::Value keyValue;
				keyValue.SetString(value.c_str(), value.size(), doc_.GetAllocator());
				optionsValue.AddMember(keyName, keyValue, doc_.GetAllocator());
			}
			params.PushBack(optionsValue, doc_.GetAllocator());
			return Send("aria2.addMetalink", params);
		}

		Result<bool> Aria2cWebSocketClient::Remove(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.remove", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			return Send("aria2.remove", params);
		}

		Result<bool> Aria2cWebSocketClient::ForceRemove(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.forceRemove", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			return Send("aria2.forceRemove", params);
		}

		Result<bool> Aria2cWebSocketClient::Pause(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.pause", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			return Send("aria2.pause", params);
		}

		Result<bool> Aria2cWebSocketClient::PauseAll() {
			// nlohmann::json params = nlohmann::json::array();
			// return Send("aria2.pauseAll", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			return Send("aria2.pauseAll", params);
		}

		Result<bool> Aria2cWebSocketClient::ForcePause(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.forcePause", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			return Send("aria2.forcePause", params);
		}

		Result<bool> Aria2cWebSocketClient::ForcePauseAll() {
			// nlohmann::json params = nlohmann::json::array();
			// return Send("aria2.forcePauseAll", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			return Send("aria2.forcePauseAll", params);
		}

		Result<bool> Aria2cWebSocketClient::Unpause(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.unpause", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			return Send("aria2.unpause", params);
		}

		Result<bool> Aria2cWebSocketClient::UnpauseAll() {
			// nlohmann::json params = nlohmann::json::array();
			// return Send("aria2.unpauseAll", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			return Send("aria2.unpauseAll", params);
		}

		Result<bool> Aria2cWebSocketClient::TellStatus(const std::string& gid, const std::vector<std::string>& keys) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// params.push_back(keys);
			// return Send("aria2.tellStatus", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			rapidjson::Value keysValue(rapidjson::kArrayType);
			for (const auto& key : keys) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				keysValue.PushBack(keyName, doc_.GetAllocator());
			}
			params.PushBack(keysValue, doc_.GetAllocator());
			return Send("aria2.tellStatus", params);
		}

		Result<bool> Aria2cWebSocketClient::GetUris(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.getUris", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			return Send("aria2.getUris", params);
		}

		Result<bool> Aria2cWebSocketClient::GetFiles(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.getFiles", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			return Send("aria2.getFiles", params);
		}

		Result<bool> Aria2cWebSocketClient::GetPeers(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.getPeers", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			return Send("aria2.getPeers", params);
		}

		Result<bool> Aria2cWebSocketClient::GetServers(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.getServers", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			return Send("aria2.getServers", params);
		}

		Result<bool> Aria2cWebSocketClient::TellActive(const std::vector<std::string>& keys) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(keys);
			// return Send("aria2.tellActive", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value keysValue(rapidjson::kArrayType);
			for (const auto& key : keys) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				keysValue.PushBack(keyName, doc_.GetAllocator());
			}
			params.PushBack(keysValue, doc_.GetAllocator());
			return Send("aria2.tellActive", params);
		}

		Result<bool> Aria2cWebSocketClient::TellWaiting(int offset, int num, const std::vector<std::string>& keys) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(offset);
			// params.push_back(num);
			// params.push_back(keys);
			// return Send("aria2.tellWaiting", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			rapidjson::Value offsetValue;
			rapidjson::Value numValue;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			params.PushBack(offsetValue.SetInt(offset), doc_.GetAllocator());
			params.PushBack(numValue.SetInt(num), doc_.GetAllocator());
			rapidjson::Value keysValue(rapidjson::kArrayType);
			for (const auto& key : keys) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				keysValue.PushBack(keyName, doc_.GetAllocator());
			}
			params.PushBack(keysValue, doc_.GetAllocator());
			return Send("aria2.tellWaiting", params);
		}

		Result<bool> Aria2cWebSocketClient::TellStopped(int offset, int num, const std::vector<std::string>& keys) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(offset);
			// params.push_back(num);
			// params.push_back(keys);
			// return Send("aria2.tellStopped", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			rapidjson::Value offsetValue;
			rapidjson::Value numValue;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			params.PushBack(offsetValue.SetInt(offset), doc_.GetAllocator());
			params.PushBack(numValue.SetInt(num), doc_.GetAllocator());
			rapidjson::Value keysValue(rapidjson::kArrayType);
			for (const auto& key : keys) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				keysValue.PushBack(keyName, doc_.GetAllocator());
			}
			params.PushBack(keysValue, doc_.GetAllocator());
			return Send("aria2.tellStopped", params);
		}

		Result<bool> Aria2cWebSocketClient::ChangePosition(const std::string& gid, int pos, int how) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// params.push_back(pos);
			// params.push_back(how);
			// return Send("aria2.changePosition", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			rapidjson::Value gidValue;
			rapidjson::Value posValue;
			rapidjson::Value howValue;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			params.PushBack(posValue.SetInt(pos), doc_.GetAllocator());
			params.PushBack(howValue.SetInt(how), doc_.GetAllocator());
			return Send("aria2.changePosition", params);
		}

		Result<bool> Aria2cWebSocketClient::GetOption(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.getOption", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			rapidjson::Value gidValue;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			return Send("aria2.getOption", params);
		}

		Result<bool> Aria2cWebSocketClient::changeOption(const std::string& gid, const Options& options) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// params.push_back(options);
			// return Send("aria2.changeOption", params);
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			rapidjson::Value gidValue;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			rapidjson::Value optionsValue(rapidjson::kObjectType);
			for (const auto& [key, value] : options) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				rapidjson::Value valueName;
				valueName.SetString(value.c_str(), value.size(), doc_.GetAllocator());
				optionsValue.AddMember(keyName, valueName, doc_.GetAllocator());
			}
			params.PushBack(optionsValue, doc_.GetAllocator());
			return Send("aria2.changeOption", params);
		}

		Result<bool> Aria2cWebSocketClient::GetGlobalOption() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			return Send("aria2.getGlobalOption", params);
		}

		Result<bool> Aria2cWebSocketClient::ChangeGlobalOption(const Options& options) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value optionsValue(rapidjson::kObjectType);
			for (const auto& [key, value] : options) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				rapidjson::Value valueName;
				valueName.SetString(value.c_str(), value.size(), doc_.GetAllocator());
				optionsValue.AddMember(keyName, valueName, doc_.GetAllocator());
			}
			params.PushBack(optionsValue, doc_.GetAllocator());
			return Send("aria2.changeGlobalOption", params);
		}

		Result<bool> Aria2cWebSocketClient::GetGlobalStat() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			return Send("aria2.getGlobalStat", params);
		}

		Result<bool> Aria2cWebSocketClient::PurgeDownloadResult() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			return Send("aria2.purgeDownloadResult", params);
		}

		Result<bool> Aria2cWebSocketClient::RemoveDownloadResult(const std::string& gid) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());
			return Send("aria2.removeDownloadResult", params);
		}

		Result<bool> Aria2cWebSocketClient::GetVersion() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			return Send("aria2.getVersion", params);
		}

		Result<bool> Aria2cWebSocketClient::Shutdown() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			return Send("aria2.shutdown", params);
		}

		Result<bool> Aria2cWebSocketClient::ForceShutdown() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());
			return Send("aria2.forceShutdown", params);
		}

		Result<bool> Aria2cWebSocketClient::Multicall(const Options& methods) {
			try {
				doc_.SetObject();
				rapidjson::Value params(rapidjson::kArrayType);
				rapidjson::Value token;
				std::string token_str = BuildRpcToken();
				token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
				params.PushBack(token, doc_.GetAllocator());

				for (const auto& method : methods) {
					rapidjson::Document methodDoc;
					methodDoc.Parse(method.second.c_str());
					if (methodDoc.HasParseError()) {
						return false;
					}

					rapidjson::Value methodObj(rapidjson::kObjectType);
					rapidjson::Value methodName;
					methodName.SetString(method.first.c_str(), method.first.size(), doc_.GetAllocator());
					methodObj.AddMember("methodName", methodName, doc_.GetAllocator());

					// 将解析后的参数复制到主文档中
					rapidjson::Value parsedParams;
					parsedParams.CopyFrom(methodDoc, doc_.GetAllocator());
					methodObj.AddMember("params", parsedParams, doc_.GetAllocator());

					params.PushBack(methodObj, doc_.GetAllocator());
				}
				return Send("system.multicall", params);
			} catch (...) {
				return false;
			}
		}

		void Aria2cWebSocketClient::SetMessageCallback(const std::function<void(const std::string&)>& cb) {
			text_message_callback_ = cb;
		}

		void Aria2cWebSocketClient::SetStateChanageCallback(const std::function<void(const State&, std::string)>& cb) {
			state_chanage_callback_ = cb;
		}

		Result<bool> Aria2cWebSocketClient::Send(const std::string_view& method, rapidjson::Value& params) {
			static int id = 0;
			doc_.AddMember("jsonrpc", "2.0", doc_.GetAllocator());
			doc_.AddMember("method", rapidjson::Value(method.data(), method.size()), doc_.GetAllocator());
			doc_.AddMember("params", params, doc_.GetAllocator());
			doc_.AddMember("id", rapidjson::Value(std::to_string(++id).c_str(), doc_.GetAllocator()),
						   doc_.GetAllocator());
			if (!websocket_->isConnected()) {
				return MakeFail(static_cast<std::int64_t>(gdl::ErrorType::kUnknownError));
			}
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			doc_.Accept(writer);
			const auto data = buffer.GetString();
			// LOG_DBG("Send: {}", data);
			return websocket_->send(data);
		}

	}  // namespace engine
}  // namespace gdl

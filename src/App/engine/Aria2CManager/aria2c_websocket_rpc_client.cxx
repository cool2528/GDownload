#include "aria2c_websocket_rpc_client.h"
#include <atomic>
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
				return "token:" + rpc_secret;
			}

			Result<bool> SendRpcMessage(const std::shared_ptr<WebSocketClient>& websocket,
										const std::string_view& method,
										rapidjson::Value& params) {
				static std::atomic<int> id{0};
				rapidjson::Document doc;
				doc.SetObject();
				auto& alloc = doc.GetAllocator();
				doc.AddMember("jsonrpc", "2.0", alloc);
				doc.AddMember("method", rapidjson::Value(method.data(), method.size()), alloc);
				doc.AddMember("params", params, alloc);
				doc.AddMember("id", rapidjson::Value(std::to_string(++id).c_str(), alloc), alloc);
				if (!websocket || !websocket->isConnected()) {
					return MakeFail(static_cast<std::int64_t>(gdl::ErrorType::kUnknownError));
				}
				rapidjson::StringBuffer buffer;
				rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
				doc.Accept(writer);
				return websocket->send(buffer.GetString());
			}

			void RequestVersion(const std::shared_ptr<WebSocketClient>& websocket) {
				rapidjson::Document doc;
				doc.SetObject();
				rapidjson::Value params(rapidjson::kArrayType);
				rapidjson::Value token;
				std::string token_str = BuildRpcToken();
				token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
				params.PushBack(token, doc.GetAllocator());
				(void)SendRpcMessage(websocket, "aria2.getVersion", params);
			}
		}  // namespace

		Aria2cWebSocketClient::Aria2cWebSocketClient(const std::string& url, boost::asio::io_context& ioc) : url_(url) {
			websocket_ = std::make_shared<WebSocketClient>(ioc);
			callback_state_ = std::make_shared<CallbackState>();
			auto callback_state = callback_state_;
			auto websocket = websocket_;
			// connected
			websocket_->setConnectCallback([callback_state, websocket] {
				std::function<void(const State&, std::string)> cb;
				{
					std::lock_guard lock(callback_state->mutex);
					if (!callback_state->alive.load()) return;
					cb = callback_state->state_chanage_callback;
				}
				if (cb) cb(State::kConnected, "");
				if (callback_state->alive.load()) RequestVersion(websocket);
			});
			// closed
			websocket_->setDisconnectCallback([callback_state] {
				std::function<void(const State&, std::string)> cb;
				{
					std::lock_guard lock(callback_state->mutex);
					if (!callback_state->alive.load()) return;
					cb = callback_state->state_chanage_callback;
				}
				if (cb) cb(State::kClosed, "");
			});
			// error message
			websocket_->setErrorCallback([callback_state](const std::string& error) {
				std::function<void(const State&, std::string)> cb;
				{
					std::lock_guard lock(callback_state->mutex);
					if (!callback_state->alive.load()) return;
					cb = callback_state->state_chanage_callback;
				}
				if (cb) cb(State::kClosed, error);
			});
			// receive message
			websocket_->setMessageCallback([callback_state](const std::string& msg) {
				std::function<void(const std::string&)> cb;
				{
					std::lock_guard lock(callback_state->mutex);
					if (!callback_state->alive.load()) return;
					cb = callback_state->text_message_callback;
				}
				if (cb) cb(msg);
			});
		}

		Aria2cWebSocketClient::~Aria2cWebSocketClient() {
			if (callback_state_) {
				callback_state_->alive.store(false);
				std::lock_guard lock(callback_state_->mutex);
				callback_state_->state_chanage_callback = nullptr;
				callback_state_->text_message_callback = nullptr;
			}
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
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value uriList(rapidjson::kArrayType);
			for (const auto& uri : uris) {
				rapidjson::Value uriValue;
				uriValue.SetString(uri.c_str(), uri.size(), doc.GetAllocator());
				uriList.PushBack(uriValue, doc.GetAllocator());
			}
			params.PushBack(uriList, doc.GetAllocator());
			rapidjson::Value optionsValue;
			optionsValue.SetObject();
			for (const auto& [key, value] : options) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc.GetAllocator());
				rapidjson::Value keyValue;
				keyValue.SetString(value.c_str(), value.size(), doc.GetAllocator());
				optionsValue.AddMember(keyName, keyValue, doc.GetAllocator());
			}
			params.PushBack(optionsValue, doc.GetAllocator());
			return Send("aria2.addUri", params);
		}

		Result<bool> Aria2cWebSocketClient::AddTorrent(const std::string& torrent, const Options& options) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(torrent);
			//          params.push_back(nlohmann::json::array());
			// params.push_back(options);
			// return Send("aria2.addTorrent", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value torrentValue;
			torrentValue.SetString(torrent.c_str(), torrent.size(), doc.GetAllocator());
			params.PushBack(torrentValue, doc.GetAllocator());
			rapidjson::Value emptyArray(rapidjson::kArrayType);
			params.PushBack(emptyArray, doc.GetAllocator());
			rapidjson::Value optionsValue;
			optionsValue.SetObject();
			for (const auto& [key, value] : options) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc.GetAllocator());
				rapidjson::Value keyValue;
				keyValue.SetString(value.c_str(), value.size(), doc.GetAllocator());
				optionsValue.AddMember(keyName, keyValue, doc.GetAllocator());
			}
			params.PushBack(optionsValue, doc.GetAllocator());
			return Send("aria2.addTorrent", params);
		}

		Result<bool> Aria2cWebSocketClient::AddMetalink(const std::string& metalink, const Options& options) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(metalink);
			// params.push_back(options);
			// return Send("aria2.addMetalink", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value metalinkValue;
			metalinkValue.SetString(metalink.c_str(), metalink.size(), doc.GetAllocator());
			params.PushBack(metalinkValue, doc.GetAllocator());
			rapidjson::Value optionsValue;
			optionsValue.SetObject();
			for (const auto& [key, value] : options) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc.GetAllocator());
				rapidjson::Value keyValue;
				keyValue.SetString(value.c_str(), value.size(), doc.GetAllocator());
				optionsValue.AddMember(keyName, keyValue, doc.GetAllocator());
			}
			params.PushBack(optionsValue, doc.GetAllocator());
			return Send("aria2.addMetalink", params);
		}

		Result<bool> Aria2cWebSocketClient::Remove(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.remove", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			return Send("aria2.remove", params);
		}

		Result<bool> Aria2cWebSocketClient::ForceRemove(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.forceRemove", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			return Send("aria2.forceRemove", params);
		}

		Result<bool> Aria2cWebSocketClient::Pause(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.pause", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			return Send("aria2.pause", params);
		}

		Result<bool> Aria2cWebSocketClient::PauseAll() {
			// nlohmann::json params = nlohmann::json::array();
			// return Send("aria2.pauseAll", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			return Send("aria2.pauseAll", params);
		}

		Result<bool> Aria2cWebSocketClient::ForcePause(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.forcePause", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			return Send("aria2.forcePause", params);
		}

		Result<bool> Aria2cWebSocketClient::ForcePauseAll() {
			// nlohmann::json params = nlohmann::json::array();
			// return Send("aria2.forcePauseAll", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			return Send("aria2.forcePauseAll", params);
		}

		Result<bool> Aria2cWebSocketClient::Unpause(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.unpause", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			return Send("aria2.unpause", params);
		}

		Result<bool> Aria2cWebSocketClient::UnpauseAll() {
			// nlohmann::json params = nlohmann::json::array();
			// return Send("aria2.unpauseAll", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			return Send("aria2.unpauseAll", params);
		}

		Result<bool> Aria2cWebSocketClient::TellStatus(const std::string& gid, const std::vector<std::string>& keys) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// params.push_back(keys);
			// return Send("aria2.tellStatus", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			rapidjson::Value keysValue(rapidjson::kArrayType);
			for (const auto& key : keys) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc.GetAllocator());
				keysValue.PushBack(keyName, doc.GetAllocator());
			}
			params.PushBack(keysValue, doc.GetAllocator());
			return Send("aria2.tellStatus", params);
		}

		Result<bool> Aria2cWebSocketClient::GetUris(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.getUris", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			return Send("aria2.getUris", params);
		}

		Result<bool> Aria2cWebSocketClient::GetFiles(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.getFiles", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			return Send("aria2.getFiles", params);
		}

		Result<bool> Aria2cWebSocketClient::GetPeers(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.getPeers", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			return Send("aria2.getPeers", params);
		}

		Result<bool> Aria2cWebSocketClient::GetServers(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.getServers", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			return Send("aria2.getServers", params);
		}

		Result<bool> Aria2cWebSocketClient::TellActive(const std::vector<std::string>& keys) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(keys);
			// return Send("aria2.tellActive", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value keysValue(rapidjson::kArrayType);
			for (const auto& key : keys) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc.GetAllocator());
				keysValue.PushBack(keyName, doc.GetAllocator());
			}
			params.PushBack(keysValue, doc.GetAllocator());
			return Send("aria2.tellActive", params);
		}

		Result<bool> Aria2cWebSocketClient::TellWaiting(int offset, int num, const std::vector<std::string>& keys) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(offset);
			// params.push_back(num);
			// params.push_back(keys);
			// return Send("aria2.tellWaiting", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			rapidjson::Value offsetValue;
			rapidjson::Value numValue;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			params.PushBack(offsetValue.SetInt(offset), doc.GetAllocator());
			params.PushBack(numValue.SetInt(num), doc.GetAllocator());
			rapidjson::Value keysValue(rapidjson::kArrayType);
			for (const auto& key : keys) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc.GetAllocator());
				keysValue.PushBack(keyName, doc.GetAllocator());
			}
			params.PushBack(keysValue, doc.GetAllocator());
			return Send("aria2.tellWaiting", params);
		}

		Result<bool> Aria2cWebSocketClient::TellStopped(int offset, int num, const std::vector<std::string>& keys) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(offset);
			// params.push_back(num);
			// params.push_back(keys);
			// return Send("aria2.tellStopped", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			rapidjson::Value offsetValue;
			rapidjson::Value numValue;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			params.PushBack(offsetValue.SetInt(offset), doc.GetAllocator());
			params.PushBack(numValue.SetInt(num), doc.GetAllocator());
			rapidjson::Value keysValue(rapidjson::kArrayType);
			for (const auto& key : keys) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc.GetAllocator());
				keysValue.PushBack(keyName, doc.GetAllocator());
			}
			params.PushBack(keysValue, doc.GetAllocator());
			return Send("aria2.tellStopped", params);
		}

		Result<bool> Aria2cWebSocketClient::ChangePosition(const std::string& gid, int pos, int how) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// params.push_back(pos);
			// params.push_back(how);
			// return Send("aria2.changePosition", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			rapidjson::Value gidValue;
			rapidjson::Value posValue;
			rapidjson::Value howValue;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			params.PushBack(posValue.SetInt(pos), doc.GetAllocator());
			params.PushBack(howValue.SetInt(how), doc.GetAllocator());
			return Send("aria2.changePosition", params);
		}

		Result<bool> Aria2cWebSocketClient::GetOption(const std::string& gid) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// return Send("aria2.getOption", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			rapidjson::Value gidValue;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			return Send("aria2.getOption", params);
		}

		Result<bool> Aria2cWebSocketClient::changeOption(const std::string& gid, const Options& options) {
			// nlohmann::json params = nlohmann::json::array();
			// params.push_back(gid);
			// params.push_back(options);
			// return Send("aria2.changeOption", params);
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			rapidjson::Value gidValue;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			rapidjson::Value optionsValue(rapidjson::kObjectType);
			for (const auto& [key, value] : options) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc.GetAllocator());
				rapidjson::Value valueName;
				valueName.SetString(value.c_str(), value.size(), doc.GetAllocator());
				optionsValue.AddMember(keyName, valueName, doc.GetAllocator());
			}
			params.PushBack(optionsValue, doc.GetAllocator());
			return Send("aria2.changeOption", params);
		}

		Result<bool> Aria2cWebSocketClient::GetGlobalOption() {
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			return Send("aria2.getGlobalOption", params);
		}

		Result<bool> Aria2cWebSocketClient::ChangeGlobalOption(const Options& options) {
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value optionsValue(rapidjson::kObjectType);
			for (const auto& [key, value] : options) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc.GetAllocator());
				rapidjson::Value valueName;
				valueName.SetString(value.c_str(), value.size(), doc.GetAllocator());
				optionsValue.AddMember(keyName, valueName, doc.GetAllocator());
			}
			params.PushBack(optionsValue, doc.GetAllocator());
			return Send("aria2.changeGlobalOption", params);
		}

		Result<bool> Aria2cWebSocketClient::GetGlobalStat() {
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			return Send("aria2.getGlobalStat", params);
		}

		Result<bool> Aria2cWebSocketClient::PurgeDownloadResult() {
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			return Send("aria2.purgeDownloadResult", params);
		}

		Result<bool> Aria2cWebSocketClient::RemoveDownloadResult(const std::string& gid) {
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc.GetAllocator());
			params.PushBack(gidValue, doc.GetAllocator());
			return Send("aria2.removeDownloadResult", params);
		}

		Result<bool> Aria2cWebSocketClient::GetVersion() {
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			return Send("aria2.getVersion", params);
		}

		Result<bool> Aria2cWebSocketClient::Shutdown() {
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			return Send("aria2.shutdown", params);
		}

		Result<bool> Aria2cWebSocketClient::ForceShutdown() {
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);
			rapidjson::Value token;
			std::string token_str = BuildRpcToken();
			token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
			params.PushBack(token, doc.GetAllocator());
			return Send("aria2.forceShutdown", params);
		}

		Result<bool> Aria2cWebSocketClient::Multicall(const Options& methods) {
			try {
				rapidjson::Document doc;
				doc.SetObject();
				std::string token_str = BuildRpcToken();

				// aria2 协议：system.multicall 参数是单元素外层数组，元素为方法调用对象数组；
				// 每个子调用 params 首位需各自带 token（E5）
				rapidjson::Value method_array(rapidjson::kArrayType);
				for (const auto& method : methods) {
					rapidjson::Document methodDoc;
					methodDoc.Parse(method.second.c_str());
					if (methodDoc.HasParseError() || !methodDoc.IsArray()) {
						return MakeFail(static_cast<std::int64_t>(gdl::ErrorType::kUnknownError), "multicall parse failed");
					}

					rapidjson::Value methodObj(rapidjson::kObjectType);
					rapidjson::Value methodName;
					methodName.SetString(method.first.c_str(), method.first.size(), doc.GetAllocator());
					methodObj.AddMember("methodName", methodName, doc.GetAllocator());

					// 子调用 params：token 置首，随后拼接原参数
					rapidjson::Value sub_params(rapidjson::kArrayType);
					rapidjson::Value token;
					token.SetString(token_str.c_str(), token_str.size(), doc.GetAllocator());
					sub_params.PushBack(token, doc.GetAllocator());
					for (auto& arg : methodDoc.GetArray()) {
						rapidjson::Value copied;
						copied.CopyFrom(arg, doc.GetAllocator());
						sub_params.PushBack(copied, doc.GetAllocator());
					}
					methodObj.AddMember("params", sub_params, doc.GetAllocator());

					method_array.PushBack(methodObj, doc.GetAllocator());
				}

				// 外层单元素数组包裹方法数组
				rapidjson::Value params(rapidjson::kArrayType);
				params.PushBack(method_array, doc.GetAllocator());
				return Send("system.multicall", params);
			} catch (const std::exception& e) {
				LOG_ERR("multicall failed: {}", e.what());
				return MakeFail(static_cast<std::int64_t>(gdl::ErrorType::kUnknownError), "multicall exception");
			} catch (...) {
				return MakeFail(static_cast<std::int64_t>(gdl::ErrorType::kUnknownError), "multicall unknown exception");
			}
		}

		void Aria2cWebSocketClient::SetMessageCallback(const std::function<void(const std::string&)>& cb) {
			std::lock_guard lock(callback_state_->mutex);
			callback_state_->text_message_callback = cb;
		}

		void Aria2cWebSocketClient::SetStateChanageCallback(const std::function<void(const State&, std::string)>& cb) {
			std::lock_guard lock(callback_state_->mutex);
			callback_state_->state_chanage_callback = cb;
		}

		Result<bool> Aria2cWebSocketClient::Send(const std::string_view& method, rapidjson::Value& params) {
			// 用局部 Document 而非共享成员 doc_，避免多线程并发 RPC 时对同一 rapidjson::Document
			// 的数据竞争（破坏 JSON 结构/崩溃）。id 改为原子，消除 ++id 的数据竞争 UB。
			// 注意：params 由调用方用其局部 allocator 构造，其字符串内存指向调用方 doc；
			// 本函数在调用方 doc 析构前同步完成序列化，访问安全。
			return SendRpcMessage(websocket_, method, params);
		}

	}  // namespace engine
}  // namespace gdl

#include "aria2c_http_rpc_client.h"
#include <cpr/cpr.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <format>
#include "engine_def.h"
#include "logger.h"
namespace gdl {
	namespace engine {

		Aria2cHttpClient::Aria2cHttpClient(std::string_view host) : host_(host) {}

		Result<Response> Aria2cHttpClient::AddUri(const std::vector<std::string>& uris, const Aria2Options& options) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add URIs
			rapidjson::Value uriArray(rapidjson::kArrayType);
			for (const auto& uri : uris) {
				rapidjson::Value uriValue;
				uriValue.SetString(uri.c_str(), uri.size(), doc_.GetAllocator());
				uriArray.PushBack(uriValue, doc_.GetAllocator());
			}
			params.PushBack(uriArray, doc_.GetAllocator());

			// Add options
			rapidjson::Value optionsValue(rapidjson::kObjectType);
			for (const auto& [key, value] : options) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				rapidjson::Value valueName;
				valueName.SetString(value.c_str(), value.size(), doc_.GetAllocator());
				optionsValue.AddMember(keyName, valueName, doc_.GetAllocator());
			}
			params.PushBack(optionsValue, doc_.GetAllocator());

			return Send("aria2.addUri", params);
		}

		Result<Response> Aria2cHttpClient::AddTorrent(const std::string& torrent, const Aria2Options& options) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add torrent
			rapidjson::Value torrentValue;
			torrentValue.SetString(torrent.c_str(), torrent.size(), doc_.GetAllocator());
			params.PushBack(torrentValue, doc_.GetAllocator());

			// Add empty array
			rapidjson::Value emptyArray(rapidjson::kArrayType);
			params.PushBack(emptyArray, doc_.GetAllocator());

			// Add options
			rapidjson::Value optionsValue(rapidjson::kObjectType);
			for (const auto& [key, value] : options) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				rapidjson::Value valueName;
				valueName.SetString(value.c_str(), value.size(), doc_.GetAllocator());
				optionsValue.AddMember(keyName, valueName, doc_.GetAllocator());
			}
			params.PushBack(optionsValue, doc_.GetAllocator());

			return Send("aria2.addTorrent", params);
		}

		Result<Response> Aria2cHttpClient::AddMetalink(const std::string& metalink, const Aria2Options& options) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add metalink
			rapidjson::Value metalinkValue;
			metalinkValue.SetString(metalink.c_str(), metalink.size(), doc_.GetAllocator());
			params.PushBack(metalinkValue, doc_.GetAllocator());

			// Add options
			rapidjson::Value optionsValue(rapidjson::kObjectType);
			for (const auto& [key, value] : options) {
				rapidjson::Value keyName;
				keyName.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				rapidjson::Value valueName;
				valueName.SetString(value.c_str(), value.size(), doc_.GetAllocator());
				optionsValue.AddMember(keyName, valueName, doc_.GetAllocator());
			}
			params.PushBack(optionsValue, doc_.GetAllocator());

			return Send("aria2.addMetalink", params);
		}

		Result<Response> Aria2cHttpClient::Remove(const std::string& gid) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			return Send("aria2.remove", params);
		}

		Result<Response> Aria2cHttpClient::ForceRemove(const std::string& gid) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			return Send("aria2.forceRemove", params);
		}

		Result<Response> Aria2cHttpClient::Pause(const std::string& gid) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			return Send("aria2.pause", params);
		}

		Result<Response> Aria2cHttpClient::PauseAll() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			return Send("aria2.pauseAll", params);
		}

		Result<Response> Aria2cHttpClient::ForcePause(const std::string& gid) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			return Send("aria2.forcePause", params);
		}

		Result<Response> Aria2cHttpClient::ForcePauseAll() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			return Send("aria2.forcePauseAll", params);
		}

		Result<Response> Aria2cHttpClient::Unpause(const std::string& gid) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			return Send("aria2.unpause", params);
		}

		Result<Response> Aria2cHttpClient::UnpauseAll() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			return Send("aria2.unpauseAll", params);
		}

		Result<Response> Aria2cHttpClient::TellStatus(const std::string& gid, const std::vector<std::string>& keys) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			// Add keys array
			rapidjson::Value keysArray(rapidjson::kArrayType);
			for (const auto& key : keys) {
				rapidjson::Value keyValue;
				keyValue.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				keysArray.PushBack(keyValue, doc_.GetAllocator());
			}
			params.PushBack(keysArray, doc_.GetAllocator());

			return Send("aria2.tellStatus", params);
		}

		Result<Response> Aria2cHttpClient::GetUris(const std::string& gid) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			return Send("aria2.getUris", params);
		}

		Result<Response> Aria2cHttpClient::GetFiles(const std::string& gid) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			return Send("aria2.getFiles", params);
		}

		Result<Response> Aria2cHttpClient::GetPeers(const std::string& gid) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			return Send("aria2.getPeers", params);
		}

		Result<Response> Aria2cHttpClient::GetServers(const std::string& gid) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			return Send("aria2.getServers", params);
		}

		Result<Response> Aria2cHttpClient::TellActive(const std::vector<std::string>& keys) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add keys array
			rapidjson::Value keysArray(rapidjson::kArrayType);
			for (const auto& key : keys) {
				rapidjson::Value keyValue;
				keyValue.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				keysArray.PushBack(keyValue, doc_.GetAllocator());
			}
			params.PushBack(keysArray, doc_.GetAllocator());

			return Send("aria2.tellActive", params);
		}

		Result<Response> Aria2cHttpClient::TellWaiting(int offset, int num, const std::vector<std::string>& keys) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add offset and num
			rapidjson::Value offsetValue;
			rapidjson::Value numValue;
			params.PushBack(offsetValue.SetInt(offset), doc_.GetAllocator());
			params.PushBack(numValue.SetInt(num), doc_.GetAllocator());

			// Add keys array
			rapidjson::Value keysArray(rapidjson::kArrayType);
			for (const auto& key : keys) {
				rapidjson::Value keyValue;
				keyValue.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				keysArray.PushBack(keyValue, doc_.GetAllocator());
			}
			params.PushBack(keysArray, doc_.GetAllocator());

			return Send("aria2.tellWaiting", params);
		}

		Result<Response> Aria2cHttpClient::TellStopped(int offset, int num, const std::vector<std::string>& keys) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add offset and num
			rapidjson::Value offsetValue;
			rapidjson::Value numValue;
			params.PushBack(offsetValue.SetInt(offset), doc_.GetAllocator());
			params.PushBack(numValue.SetInt(num), doc_.GetAllocator());

			// Add keys array
			rapidjson::Value keysArray(rapidjson::kArrayType);
			for (const auto& key : keys) {
				rapidjson::Value keyValue;
				keyValue.SetString(key.c_str(), key.size(), doc_.GetAllocator());
				keysArray.PushBack(keyValue, doc_.GetAllocator());
			}
			params.PushBack(keysArray, doc_.GetAllocator());

			return Send("aria2.tellStopped", params);
		}

		Result<Response> Aria2cHttpClient::ChangePosition(const std::string& gid, int pos, int how) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			// Add pos and how
			rapidjson::Value posValue;
			rapidjson::Value howValue;
			params.PushBack(posValue.SetInt(pos), doc_.GetAllocator());
			params.PushBack(howValue.SetInt(how), doc_.GetAllocator());

			return Send("aria2.changePosition", params);
		}

		Result<Response> Aria2cHttpClient::GetOption(const std::string& gid) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			return Send("aria2.getOption", params);
		}

		Result<Response> Aria2cHttpClient::changeOption(const std::string& gid, const Aria2Options& options) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			// Add options
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

		Result<Response> Aria2cHttpClient::GetGlobalOption() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			return Send("aria2.getGlobalOption", params);
		}

		Result<Response> Aria2cHttpClient::ChangeGlobalOption(const Aria2Options& options) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add options
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

		Result<Response> Aria2cHttpClient::GetGlobalStat() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			return Send("aria2.getGlobalStat", params);
		}

		Result<Response> Aria2cHttpClient::PurgeDownloadResult() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			return Send("aria2.purgeDownloadResult", params);
		}

		Result<Response> Aria2cHttpClient::RemoveDownloadResult(const std::string& gid) {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			// Add gid
			rapidjson::Value gidValue;
			gidValue.SetString(gid.c_str(), gid.size(), doc_.GetAllocator());
			params.PushBack(gidValue, doc_.GetAllocator());

			return Send("aria2.removeDownloadResult", params);
		}

		Result<Response> Aria2cHttpClient::GetVersion() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			return Send("aria2.getVersion", params);
		}

		Result<Response> Aria2cHttpClient::Shutdown() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			return Send("aria2.shutdown", params);
		}

		Result<Response> Aria2cHttpClient::ForceShutdown() {
			doc_.SetObject();
			rapidjson::Value params(rapidjson::kArrayType);

			// Add token
			rapidjson::Value token;
            std::string token_str = std::format("token:{}", kDefaultRpcSecret);
			token.SetString(token_str.c_str(), token_str.size(), doc_.GetAllocator());
			params.PushBack(token, doc_.GetAllocator());

			return Send("aria2.forceShutdown", params);
		}

		Result<Response> Aria2cHttpClient::Send(const std::string_view& method, rapidjson::Value& params) {
			Response result;
			doc_.AddMember("jsonrpc", "2.0", doc_.GetAllocator());
            doc_.AddMember("method", rapidjson::Value(method.data(), method.size(), doc_.GetAllocator()),
                           doc_.GetAllocator());
			doc_.AddMember("params", params, doc_.GetAllocator());
            doc_.AddMember("id", rapidjson::Value(std::to_string(++id_).c_str(), doc_.GetAllocator()),
                           doc_.GetAllocator());

			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			doc_.Accept(writer);
			const std::string body = buffer.GetString();
            auto reply			   = cpr::Post(cpr::Url(host_ + "/jsonrpc"), cpr::Body(body),
                                               cpr::Header{{"Content-Type", "application/json"}});
			if (reply.status_code != 200) {
				LOG_ERR("request aria2c method fail error {}", reply.error.message);
                result.result = ErrorResult{.err_msg  = reply.error.message,
                                            .err_code = static_cast<std::int64_t>(reply.status_code)};
				return result;
			}
            result.result	  = SucceedResult{.body = std::move(reply.text)};
			result.is_succeed = true;
			return result;
		}

	}  // namespace engine
}  // namespace gdl

#include "aria2c_http_rpc_client.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#ifdef __APPLE__
#define CPPHTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN
#endif
#include <httplib.h>
#include <format>
#include "engine_def.h"
#include "logger.h"
namespace gdl {
	namespace engine {

		Aria2cHttpClient::Aria2cHttpClient(std::string_view host) : host_(host) {}

		Result<Response> Aria2cHttpClient::AddUri(const std::vector<std::string>& uris, const Aria2Options& options) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(uris);
			params.push_back(options);
			return Send("aria2.addUri", params);
		}

		Result<Response> Aria2cHttpClient::AddTorrent(const std::string& torrent, const Aria2Options& options) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(torrent);
			params.push_back(options);
			return Send("aria2.addTorrent", params);
		}

		Result<Response> Aria2cHttpClient::AddMetalink(const std::string& metalink, const Aria2Options& options) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(metalink);
			params.push_back(options);
			return Send("aria2.addMetalink", params);
		}

		Result<Response> Aria2cHttpClient::Remove(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.remove", params);
		}

		Result<Response> Aria2cHttpClient::ForceRemove(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.forceRemove", params);
		}

		Result<Response> Aria2cHttpClient::Pause(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.pause", params);
		}

		Result<Response> Aria2cHttpClient::PauseAll() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.pauseAll", params);
		}

		Result<Response> Aria2cHttpClient::ForcePause(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.forcePause", params);
		}

		Result<Response> Aria2cHttpClient::ForcePauseAll() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.forcePauseAll", params);
		}

		Result<Response> Aria2cHttpClient::Unpause(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.unpause", params);
		}

		Result<Response> Aria2cHttpClient::UnpauseAll() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.unpauseAll", params);
		}

		Result<Response> Aria2cHttpClient::TellStatus(const std::string& gid, const std::vector<std::string>& keys) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			params.push_back(keys);
			return Send("aria2.tellStatus", params);
		}

		Result<Response> Aria2cHttpClient::GetUris(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.getUris", params);
		}

		Result<Response> Aria2cHttpClient::GetFiles(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.getFiles", params);
		}

		Result<Response> Aria2cHttpClient::GetPeers(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.getPeers", params);
		}

		Result<Response> Aria2cHttpClient::GetServers(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.getServers", params);
		}

		Result<Response> Aria2cHttpClient::TellActive(const std::vector<std::string>& keys) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(keys);
			return Send("aria2.tellActive", params);
		}

		Result<Response> Aria2cHttpClient::TellWaiting(int offset, int num, const std::vector<std::string>& keys) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(offset);
			params.push_back(num);
			params.push_back(keys);
			return Send("aria2.tellWaiting", params);
		}

		Result<Response> Aria2cHttpClient::TellStopped(int offset, int num, const std::vector<std::string>& keys) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(offset);
			params.push_back(num);
			params.push_back(keys);
			return Send("aria2.tellStopped", params);
		}

		Result<Response> Aria2cHttpClient::ChangePosition(const std::string& gid, int pos, int how) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			params.push_back(pos);
			params.push_back(how);
			return Send("aria2.changePosition", params);
		}

		Result<Response> Aria2cHttpClient::GetOption(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.getOption", params);
		}

		Result<Response> Aria2cHttpClient::changeOption(const std::string& gid, const Aria2Options& options) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			params.push_back(options);
			return Send("aria2.changeOption", params);
		}

		Result<Response> Aria2cHttpClient::GetGlobalOption() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.getGlobalOption", params);
		}

		Result<Response> Aria2cHttpClient::ChangeGlobalOption(const Aria2Options& options) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(options);
			return Send("aria2.changeGlobalOption", params);
		}

		Result<Response> Aria2cHttpClient::GetGlobalStat() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.getGlobalStat", params);
		}

		Result<Response> Aria2cHttpClient::PurgeDownloadResult() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.purgeDownloadResult", params);
		}

		Result<Response> Aria2cHttpClient::RemoveDownloadResult(const std::string& gid) {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			params.push_back(gid);
			return Send("aria2.removeDownloadResult", params);
		}

		Result<Response> Aria2cHttpClient::GetVersion() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.getVersion", params);
		}

		Result<Response> Aria2cHttpClient::Shutdown() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.shutdown", params);
		}

		Result<Response> Aria2cHttpClient::ForceShutdown() {
			nlohmann::json params = nlohmann::json::array();
			params.push_back(std::format("token:{}", kDefaultRpcSecret));
			return Send("aria2.forceShutdown", params);
		}

		Result<Response> Aria2cHttpClient::Send(const std::string_view& method, const nlohmann::json& params) {
			Response result;
			nlohmann::json doc;
			doc["jsonrpc"] = "2.0";
			doc["method"]  = method;
			doc["params"]  = params;
			doc["id"]	   = std::to_string(++id_);
			httplib::Client cli(host_);
			httplib::Headers headers;
			headers.insert(std::make_pair("Content-Type", "application/json"));

			auto reply = cli.Post("/jsonrpc", headers, doc.dump(), "application/json");
			if (!reply) {
				LOG_ERR("request aria2c method fail error {}", httplib::to_string(reply.error()));
				result.result = ErrorResult{.err_msg  = httplib::to_string(reply.error()),
											.err_code = static_cast<std::int64_t>(reply.error())};
				return result;
			}
			else if (reply.value().status != 200) {
				LOG_ERR("request aria2c method fail error {}", httplib::to_string(reply.error()));
				result.result = ErrorResult{.err_msg  = httplib::to_string(reply.error()),
											.err_code = static_cast<std::int64_t>(reply.error())};
				return result;
			}
			result.is_succeed = true;
			result.result	  = SucceedResult{.body = std::move(reply.value().body)};
			return result;
		}

	}  // namespace engine
}  // namespace gdl

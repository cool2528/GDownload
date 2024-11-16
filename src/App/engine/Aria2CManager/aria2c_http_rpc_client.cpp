#include "aria2c_http_rpc_client.h"
namespace gdl {
	namespace engine {

		Aria2cClient::Aria2cClient(const std::string_view& host) : host_(host) {}

		Result<Response> Aria2cClient::AddUri(const std::string_view& uri, const Aria2Options& options) {
			return MakeFail(static_cast<std::int64_t>(ErrorType::kUnknownError));
		}

		Result<nlohmann::json> Aria2cClient::Send(const nlohmann::json& data) {
			nlohmann::json doc;
			return doc;
		}

	}  // namespace engine
}  // namespace gdl

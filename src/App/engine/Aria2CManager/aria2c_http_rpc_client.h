#pragma once
#include <nlohmann/json.hpp>
#include "GDLCore/result/result.h"

namespace gdl {
	namespace engine {
		struct Response {
			std::string body;
			std::int64_t code;
			bool is_succeed{false};
		};
		using Aria2Options = std::unordered_multimap<std::string, std::string>;
		class Aria2cClient {
		   public:
			explicit Aria2cClient(const std::string_view& host);
			~Aria2cClient() = default;
			Result<Response> AddUri(const std::string_view& uri, const Aria2Options& options);

		   private:
			Result<nlohmann::json> Send(const nlohmann::json& data);

		   private:
			std::string_view host_;
		};
	}  // namespace engine
}  // namespace gdl

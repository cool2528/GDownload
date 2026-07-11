#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "export.h"

namespace gdl::update {
	enum class RedirectChainDecision { kFollow, kReject, kLoop, kTooManyRedirects };

	struct RedirectChainResult {
		RedirectChainDecision decision{RedirectChainDecision::kReject};
		std::string url;
	};

	class GDLCore_API RedirectChainController {
	public:
		RedirectChainController(std::string initial_url, std::vector<std::string> allowed_hosts,
			std::size_t max_redirects = 5);

		RedirectChainResult Follow(std::string_view location);

	private:
		std::string current_url_;
		std::vector<std::string> allowed_hosts_;
		std::unordered_set<std::string> visited_urls_;
		std::size_t redirect_count_{0};
		std::size_t max_redirects_{5};
	};
}  // namespace gdl::update

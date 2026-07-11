#include "redirect_chain_controller.h"

#include <boost/url.hpp>

#include "update_url_policy.h"

namespace gdl::update {
	RedirectChainController::RedirectChainController(std::string initial_url,
		std::vector<std::string> allowed_hosts, std::size_t max_redirects)
		: current_url_(std::move(initial_url)),
		  allowed_hosts_(std::move(allowed_hosts)),
		  max_redirects_(max_redirects) {
		visited_urls_.insert(current_url_);
	}

	RedirectChainResult RedirectChainController::Follow(std::string_view location) {
		if (redirect_count_ >= max_redirects_)
			return {RedirectChainDecision::kTooManyRedirects, {}};

		auto base = boost::urls::parse_uri(current_url_);
		auto reference = boost::urls::parse_uri_reference(location);
		if (!base || !reference) return {RedirectChainDecision::kReject, {}};

		boost::urls::url resolved(*base);
		if (auto result = resolved.resolve(*reference); !result)
			return {RedirectChainDecision::kReject, {}};
		const auto target = resolved.buffer();
		if (!ValidateDownloadUrl(target, allowed_hosts_))
			return {RedirectChainDecision::kReject, {}};
		if (visited_urls_.contains(target)) return {RedirectChainDecision::kLoop, {}};

		++redirect_count_;
		current_url_ = target;
		visited_urls_.insert(current_url_);
		return {RedirectChainDecision::kFollow, current_url_};
	}
}  // namespace gdl::update

#include "update_url_policy.h"

#include <algorithm>
#include <cctype>

#include <boost/url.hpp>

namespace gdl::update {
	namespace {
		std::string LowerAscii(std::string value) {
			std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
				return static_cast<char>(std::tolower(character));
			});
			return value;
		}
	}  // namespace

	bool ValidateDownloadUrl(std::string_view url, const std::vector<std::string>& allowed_hosts) {
		const auto parsed = boost::urls::parse_uri({url.data(), url.size()});
		if (!parsed || parsed->scheme_id() != boost::urls::scheme::https || !parsed->has_authority() ||
			parsed->has_userinfo())
			return false;
		if (!parsed->port().empty() && parsed->port() != "443") return false;

		const auto host = LowerAscii(parsed->host());
		return !host.empty() && std::find(allowed_hosts.begin(), allowed_hosts.end(), host) != allowed_hosts.end();
	}

	RedirectDecision DecideDownloadRedirect(std::string_view redirect_url,
		const std::vector<std::string>& allowed_hosts) {
		return ValidateDownloadUrl(redirect_url, allowed_hosts) ? RedirectDecision::kFollow : RedirectDecision::kReject;
	}
}  // namespace gdl::update

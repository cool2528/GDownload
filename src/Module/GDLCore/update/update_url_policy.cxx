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

	std::vector<std::string> BuildAllowedDownloadHosts(bool use_mirror) {
		std::vector<std::string> hosts{"github.com", "objects.githubusercontent.com",
			"release-assets.githubusercontent.com", "gdownload.uk"};
		if (use_mirror) hosts.emplace_back(kGithubMirrorHost);
		return hosts;
	}

	std::string ResolveDownloadUrl(const std::string& original_url, bool use_mirror) {
		if (!use_mirror || original_url.empty()) return original_url;
		// 已带镜像前缀不重复叠加
		if (original_url.rfind(kGithubMirrorPrefix, 0) == 0) return original_url;

		// 严格按 host 判定 GitHub 资源，避免 query/path 中出现域名字样被误判
		const auto parsed = boost::urls::parse_uri(original_url);
		if (!parsed || parsed->scheme_id() != boost::urls::scheme::https) return original_url;
		const auto host = LowerAscii(parsed->host());
		const auto is_suffix = [&host](std::string_view suffix) {
			return host.size() > suffix.size() &&
				   host.compare(host.size() - suffix.size(), suffix.size(), suffix) == 0;
		};
		const bool is_github_resource = host == "github.com" || host == "githubusercontent.com" ||
										is_suffix(".github.com") || is_suffix(".githubusercontent.com");
		if (!is_github_resource) return original_url;

		return std::string(kGithubMirrorPrefix) + original_url;
	}
}  // namespace gdl::update

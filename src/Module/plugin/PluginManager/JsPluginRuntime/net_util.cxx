#include "net_util.h"

#include <algorithm>
#include <boost/url.hpp>

namespace gdl {
	namespace plugin {
		namespace js {

			namespace {
				std::string ToLower(std::string_view s) {
					std::string result(s);
					std::transform(result.begin(), result.end(), result.begin(),
								   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
					return result;
				}
			}  // namespace

			bool ParseUrlParts(std::string_view url, UrlParts& parts_out) {
				// 仅解析 query 之前的部分提取 scheme/host/path：网盘 PCS 直链的 query 常含
				// 未编码的 '|' 等字符（如 devuid=MD5|0），boost::url 严格模式会拒绝整串，
				// 但这些字符对 host 白名单校验无影响，且 cpr/libcurl 能容忍原样发送。
				auto query_pos		 = url.find('?');
				std::string_view base = query_pos == std::string_view::npos ? url : url.substr(0, query_pos);
				auto parsed			 = boost::urls::parse_uri(base);
				if (!parsed) {
					return false;
				}
				parts_out.host	   = ToLower(parsed->host());
				parts_out.path	   = parsed->path().empty() ? "/" : std::string(parsed->path());
				parts_out.is_https = ToLower(parsed->scheme()) == "https";
				return !parts_out.host.empty();
			}

			bool HostMatchesWhitelist(std::string_view host, const std::vector<std::string>& whitelist) {
				auto lower_host = ToLower(host);
				for (const auto& entry : whitelist) {
					auto lower_entry = ToLower(entry);
					if (lower_entry.rfind("*.", 0) == 0) {
						// 子域通配：*.foo.com 匹配 a.foo.com / a.b.foo.com，不匹配 foo.com
						auto suffix = lower_entry.substr(1);  // ".foo.com"
						if (lower_host.size() > suffix.size()
							&& lower_host.compare(lower_host.size() - suffix.size(), suffix.size(), suffix) == 0) {
							return true;
						}
					} else if (lower_host == lower_entry) {
						return true;
					}
				}
				return false;
			}

			bool DomainMatches(std::string_view host, std::string_view cookie_domain) {
				auto lower_host	  = ToLower(host);
				auto lower_domain = ToLower(cookie_domain);
				if (lower_host == lower_domain) {
					return true;
				}
				// host 以 ".domain" 结尾
				if (lower_host.size() > lower_domain.size()) {
					auto pos = lower_host.size() - lower_domain.size();
					return lower_host.compare(pos, lower_domain.size(), lower_domain) == 0
						   && lower_host[pos - 1] == '.';
				}
				return false;
			}

			bool PathMatches(std::string_view request_path, std::string_view cookie_path) {
				if (request_path == cookie_path) {
					return true;
				}
				if (request_path.size() > cookie_path.size()
					&& request_path.compare(0, cookie_path.size(), cookie_path) == 0) {
					// cookie path 以 / 结尾，或请求路径在 cookie path 后紧跟 /
					return cookie_path.back() == '/' || request_path[cookie_path.size()] == '/';
				}
				return false;
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

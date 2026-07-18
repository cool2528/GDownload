#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace gdl {
	namespace plugin {
		namespace js {

			// 从 URL 提取的关键组成部分
			struct UrlParts {
				std::string host;
				std::string path;
				bool is_https{false};
			};

			// 解析 URL 的 host/path/scheme；解析失败返回 false
			bool ParseUrlParts(std::string_view url, UrlParts& parts_out);

			// 判断主机名是否命中白名单
			// 规则：普通条目精确匹配（大小写不敏感）；"*.foo.com" 仅匹配子域（如 a.foo.com），不匹配 foo.com 本身
			bool HostMatchesWhitelist(std::string_view host, const std::vector<std::string>& whitelist);

			// RFC 6265 域匹配：cookie 域为 host 本身或其父域后缀
			bool DomainMatches(std::string_view host, std::string_view cookie_domain);

			// RFC 6265 路径匹配
			bool PathMatches(std::string_view request_path, std::string_view cookie_path);

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

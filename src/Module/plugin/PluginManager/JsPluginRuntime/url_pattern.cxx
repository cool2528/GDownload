#include "url_pattern.h"

#include <spdlog/spdlog.h>

namespace gdl {
	namespace plugin {
		namespace js {

			namespace {
				// 通配符模式转正则：* → .*，其余正则元字符转义
				std::string WildcardToRegex(const std::string& pattern) {
					std::string regex_str;
					regex_str.reserve(pattern.size() * 2);
					for (char c : pattern) {
						switch (c) {
							case '*':
								regex_str += ".*";
								break;
							case '.':
							case '+':
							case '?':
							case '^':
							case '$':
							case '(':
							case ')':
							case '[':
							case ']':
							case '{':
							case '}':
							case '|':
							case '\\':
							case '/':
								regex_str += '\\';
								regex_str += c;
								break;
							default:
								regex_str += c;
								break;
						}
					}
					return regex_str;
				}
			}  // namespace

			UrlPatternSet::UrlPatternSet(const std::vector<std::string>& patterns) {
				for (const auto& pattern : patterns) {
					try {
						regexes_.emplace_back(WildcardToRegex(pattern),
											  std::regex::ECMAScript | std::regex::icase | std::regex::optimize);
					} catch (const std::regex_error& e) {
						spdlog::warn("[js-plugin] invalid url pattern '{}': {}", pattern, e.what());
					}
				}
			}

			bool UrlPatternSet::Matches(std::string_view url) const {
				for (const auto& regex : regexes_) {
					if (std::regex_match(url.begin(), url.end(), regex)) {
						return true;
					}
				}
				return false;
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

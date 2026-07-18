#pragma once
#include <regex>
#include <string>
#include <string_view>
#include <vector>

namespace gdl {
	namespace plugin {
		namespace js {

			// manifest url_patterns 的通配符匹配器
			// 模式格式：*://pan.baidu.com/s/*，* 匹配任意字符序列（不含空串限制）
			// 编译为正则并缓存，供 CanHandle 快速路径使用
			class UrlPatternSet {
			   public:
				// 逐条编译模式；非法模式跳过并记录日志
				explicit UrlPatternSet(const std::vector<std::string>& patterns);

				// 任一模式命中即为 true
				bool Matches(std::string_view url) const;

				bool empty() const { return regexes_.empty(); }

			   private:
				std::vector<std::regex> regexes_;
			};

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

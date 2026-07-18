#pragma once
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace gdl {
	namespace plugin {
		namespace js {

			// 单语言的本地化文案
			struct LocaleStrings {
				std::string display_name;
				std::string description;
			};

			// manifest.json 的解析结果（设计文档 4.2 规范）
			struct PluginManifest {
				int manifest_version{0};
				std::string name;
				std::string display_name;
				std::string version;
				std::string author;
				std::string description;
				std::string homepage;
				std::string entry;
				std::string type;  // 本期仅支持 "netdisk"
				std::vector<std::string> url_patterns;

				struct Permissions {
					std::vector<std::string> http_domains;	// 域名白名单（支持 *.domain.com 子域通配）
					bool storage{false};
					bool verification_ui{false};
				} permissions;

				std::string min_app_version;

				// 本地化文案：locale（如 zh_CN）-> {display_name, description}，缺失回退默认字段
				std::map<std::string, LocaleStrings> locales;

				// 插件目录（加载时填充，非 manifest 字段）
				std::filesystem::path plugin_dir;

				// 入口脚本绝对路径
				std::filesystem::path EntryPath() const { return plugin_dir / entry; }

				// 按 locale 取本地化 display_name/description，缺失回退默认
				std::string LocalizedDisplayName(const std::string& locale) const {
					auto it = locales.find(locale);
					if (it != locales.end() && !it->second.display_name.empty()) {
						return it->second.display_name;
					}
					return display_name;
				}
				std::string LocalizedDescription(const std::string& locale) const {
					auto it = locales.find(locale);
					if (it != locales.end() && !it->second.description.empty()) {
						return it->second.description;
					}
					return description;
				}
			};

			// 解析并校验 manifest.json
			// 校验项：必填字段、manifest_version 支持范围、min_app_version 与宿主版本比对、
			//         permissions.http 禁止 "*" 全通配、entry 不允许路径逃逸
			// 失败返回 nullopt，错误说明写入 error_out
			std::optional<PluginManifest> LoadManifest(const std::filesystem::path& plugin_dir, std::string& error_out);

			// 语义化版本比较：a < b 返回 -1，相等 0，a > b 返回 1（仅比较 major.minor.patch 数字段）
			int CompareSemver(const std::string& a, const std::string& b);

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

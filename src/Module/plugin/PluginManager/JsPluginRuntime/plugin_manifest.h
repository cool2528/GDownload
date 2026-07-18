#pragma once
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "PluginManager_export.h"

namespace gdl {
	namespace plugin {
		namespace js {

			// 单语言的本地化文案
			struct LocaleStrings {
				std::string display_name;
				std::string description;
			};

			// 配置字段的本地化文案
			struct SettingFieldLocale {
				std::string label;
				std::string hint;
			};

			// manifest settings[] 中一个声明式配置字段（设计文档第 2 节）
			// 宿主按 type 统一渲染表单；role=token 字段的值作为 parseUrl 的 userToken
			struct SettingField {
				std::string key;	// [a-z0-9_]+，插件内唯一
				std::string type;	// text / password / textarea / bool / select / number
				std::string label;	// 默认（英文）标签
				std::string hint;	// 输入提示，可空
				bool required{false};
				std::string role;					// 空或 "token"
				std::string default_json;			// 缺省值（JSON 序列化文本，空串表示无）
				std::vector<std::string> options;	// select 的枚举值
				std::map<std::string, SettingFieldLocale> locales;

				std::string LocalizedLabel(const std::string& locale) const {
					auto it = locales.find(locale);
					if (it != locales.end() && !it->second.label.empty()) {
						return it->second.label;
					}
					return label;
				}
				std::string LocalizedHint(const std::string& locale) const {
					auto it = locales.find(locale);
					if (it != locales.end() && !it->second.hint.empty()) {
						return it->second.hint;
					}
					return hint;
				}
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

				// 声明式配置 Schema（可空；宿主统一渲染配置表单）
				std::vector<SettingField> settings;

				// role=token 的字段（至多一个，LoadManifest 已校验）；无则返回 nullptr
				const SettingField* TokenField() const {
					for (const auto& field : settings) {
						if (field.role == "token") {
							return &field;
						}
					}
					return nullptr;
				}

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
			PluginManager_API std::optional<PluginManifest> LoadManifest(const std::filesystem::path& plugin_dir,
																		 std::string& error_out);

			// 语义化版本比较：a < b 返回 -1，相等 0，a > b 返回 1（仅比较 major.minor.patch 数字段）
			PluginManager_API int CompareSemver(const std::string& a, const std::string& b);

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

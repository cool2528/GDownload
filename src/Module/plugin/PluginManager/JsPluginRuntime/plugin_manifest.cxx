#include "plugin_manifest.h"

#include <version.h>

#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace gdl {
	namespace plugin {
		namespace js {

			namespace {
				constexpr int kSupportedManifestVersion = 1;

				// 解析版本号的数字段（忽略非数字后缀）
				std::vector<int> ParseVersionParts(const std::string& version) {
					std::vector<int> parts;
					std::istringstream iss(version);
					std::string part;
					while (std::getline(iss, part, '.') && parts.size() < 3) {
						try {
							parts.push_back(std::stoi(part));
						} catch (...) {
							parts.push_back(0);
						}
					}
					while (parts.size() < 3) {
						parts.push_back(0);
					}
					return parts;
				}
			}  // namespace

			int CompareSemver(const std::string& a, const std::string& b) {
				auto pa = ParseVersionParts(a);
				auto pb = ParseVersionParts(b);
				for (size_t i = 0; i < 3; ++i) {
					if (pa[i] < pb[i]) {
						return -1;
					}
					if (pa[i] > pb[i]) {
						return 1;
					}
				}
				return 0;
			}

			std::optional<PluginManifest> LoadManifest(const std::filesystem::path& plugin_dir,
													   std::string& error_out) {
				auto manifest_path = plugin_dir / "manifest.json";
				std::ifstream file(manifest_path);
				if (!file.is_open()) {
					error_out = "manifest.json not found";
					return std::nullopt;
				}

				nlohmann::json json;
				try {
					file >> json;
				} catch (const nlohmann::json::exception& e) {
					error_out = std::string("manifest.json parse error: ") + e.what();
					return std::nullopt;
				}

				PluginManifest manifest;
				manifest.plugin_dir = plugin_dir;

				// 必填字段校验
				try {
					manifest.manifest_version = json.at("manifest_version").get<int>();
					manifest.name			  = json.at("name").get<std::string>();
					manifest.version		  = json.at("version").get<std::string>();
					manifest.entry			  = json.at("entry").get<std::string>();
					manifest.type			  = json.at("type").get<std::string>();
					manifest.url_patterns	  = json.at("url_patterns").get<std::vector<std::string>>();

					const auto& perms				  = json.at("permissions");
					manifest.permissions.http_domains = perms.at("http").get<std::vector<std::string>>();
					manifest.permissions.storage	  = perms.value("storage", false);
					manifest.permissions.verification_ui = perms.value("verification_ui", false);
				} catch (const nlohmann::json::exception& e) {
					error_out = std::string("manifest.json missing required field: ") + e.what();
					return std::nullopt;
				}

				// 可选字段
				manifest.display_name	 = json.value("display_name", manifest.name);
				manifest.author			 = json.value("author", "");
				manifest.description	 = json.value("description", "");
				manifest.homepage		 = json.value("homepage", "");
				manifest.min_app_version = json.value("min_app_version", "");

				// 可选本地化文案 locales: { "zh_CN": {display_name, description}, ... }
				if (json.contains("locales") && json["locales"].is_object()) {
					for (auto it = json["locales"].begin(); it != json["locales"].end(); ++it) {
						if (!it.value().is_object()) {
							continue;
						}
						LocaleStrings ls;
						ls.display_name			 = it.value().value("display_name", "");
						ls.description			 = it.value().value("description", "");
						manifest.locales[it.key()] = std::move(ls);
					}
				}

				// 可选声明式配置 settings（设计文档第 2 节）
				if (json.contains("settings")) {
					if (!json["settings"].is_array()) {
						error_out = "settings must be an array";
						return std::nullopt;
					}
					int token_count = 0;
					for (const auto& item : json["settings"]) {
						if (!item.is_object()) {
							error_out = "settings item must be an object";
							return std::nullopt;
						}
						SettingField field;
						try {
							field.key	= item.at("key").get<std::string>();
							field.type	= item.at("type").get<std::string>();
							field.label = item.at("label").get<std::string>();
						} catch (const nlohmann::json::exception& e) {
							error_out = std::string("settings item missing required field: ") + e.what();
							return std::nullopt;
						}
						// key 格式：[a-z0-9_]+
						if (field.key.empty()) {
							error_out = "settings key is empty";
							return std::nullopt;
						}
						for (char c : field.key) {
							bool valid = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
							if (!valid) {
								error_out = "invalid settings key (must be [a-z0-9_]+): " + field.key;
								return std::nullopt;
							}
						}
						// key 唯一
						for (const auto& existing : manifest.settings) {
							if (existing.key == field.key) {
								error_out = "duplicated settings key: " + field.key;
								return std::nullopt;
							}
						}
						// type 合法性
						const bool type_ok = field.type == "text" || field.type == "password"
											 || field.type == "textarea" || field.type == "bool"
											 || field.type == "select" || field.type == "number";
						if (!type_ok) {
							error_out = "unsupported settings type: " + field.type;
							return std::nullopt;
						}
						// 可选字段类型可能被第三方 manifest 写错（如 required 传字符串），需捕获异常避免崩溃
						try {
							field.hint	   = item.value("hint", "");
							field.required = item.value("required", false);
							field.role	   = item.value("role", "");
						} catch (const nlohmann::json::exception& e) {
							error_out = std::string("settings item invalid field type: ") + e.what();
							return std::nullopt;
						}
						if (!field.role.empty() && field.role != "token") {
							error_out = "unsupported settings role: " + field.role;
							return std::nullopt;
						}
						if (field.role == "token") {
							++token_count;
							// token 值要作为字符串传给 parseUrl，仅允许文本类字段
							const bool token_type_ok =
								field.type == "text" || field.type == "password" || field.type == "textarea";
							if (!token_type_ok) {
								error_out = "role=token requires a text-like type, got: " + field.type;
								return std::nullopt;
							}
						}
						if (item.contains("default")) {
							field.default_json = item["default"].dump();
						}
						if (item.contains("options") && item["options"].is_array()) {
							try {
								field.options = item["options"].get<std::vector<std::string>>();
							} catch (const nlohmann::json::exception& e) {
								error_out = std::string("settings item invalid field type: ") + e.what();
								return std::nullopt;
							}
						}
						if (field.type == "select" && field.options.empty()) {
							error_out = "settings select field requires non-empty options: " + field.key;
							return std::nullopt;
						}
						// 本地化 label/hint（同样需要防御字段类型错误）
						if (item.contains("locales") && item["locales"].is_object()) {
							for (auto it = item["locales"].begin(); it != item["locales"].end(); ++it) {
								if (!it.value().is_object()) {
									continue;
								}
								SettingFieldLocale locale_strings;
								try {
									locale_strings.label = it.value().value("label", "");
									locale_strings.hint	 = it.value().value("hint", "");
								} catch (const nlohmann::json::exception& e) {
									error_out = std::string("settings item invalid field type: ") + e.what();
									return std::nullopt;
								}
								field.locales[it.key()] = std::move(locale_strings);
							}
						}
						// 可选获取引导 help：畸形时整体忽略（不因此拒载插件）
						if (item.contains("help") && item["help"].is_object()) {
							const auto& help_json = item["help"];
							SettingFieldHelp help;
							try {
								if (help_json.contains("steps") && help_json["steps"].is_array()) {
									help.steps = help_json["steps"].get<std::vector<std::string>>();
								}
								if (help_json.contains("url") && help_json["url"].is_string()) {
									help.url = help_json["url"].get<std::string>();
								}
								if (help_json.contains("locales") && help_json["locales"].is_object()) {
									for (auto lit = help_json["locales"].begin(); lit != help_json["locales"].end();
										 ++lit) {
										if (!lit.value().is_object()) {
											continue;
										}
										if (lit.value().contains("steps") && lit.value()["steps"].is_array()) {
											help.locale_steps[lit.key()] =
												lit.value()["steps"].get<std::vector<std::string>>();
										}
									}
								}
							} catch (const nlohmann::json::exception&) {
								help = SettingFieldHelp{};
							}
							if (!help.steps.empty() || !help.url.empty()) {
								field.help = std::move(help);
							}
						}
						manifest.settings.push_back(std::move(field));
					}
					if (token_count > 1) {
						error_out = "at most one settings field may have role=token";
						return std::nullopt;
					}
				}

				// manifest_version 支持范围
				if (manifest.manifest_version != kSupportedManifestVersion) {
					error_out = "unsupported manifest_version: " + std::to_string(manifest.manifest_version);
					return std::nullopt;
				}

				// 插件类型（本期仅 netdisk）
				if (manifest.type != "netdisk") {
					error_out = "unsupported plugin type: " + manifest.type;
					return std::nullopt;
				}

				// name 格式：kebab-case（小写字母/数字/连字符）
				for (char c : manifest.name) {
					bool valid = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
					if (!valid) {
						error_out = "invalid plugin name (must be kebab-case): " + manifest.name;
						return std::nullopt;
					}
				}
				if (manifest.name.empty()) {
					error_out = "plugin name is empty";
					return std::nullopt;
				}

				// url_patterns 非空
				if (manifest.url_patterns.empty()) {
					error_out = "url_patterns is empty";
					return std::nullopt;
				}

				// http 白名单：禁止全通配
				for (const auto& domain : manifest.permissions.http_domains) {
					if (domain == "*" || domain == "*.*" || domain.empty()) {
						error_out = "permissions.http must not contain wildcard-all: " + domain;
						return std::nullopt;
					}
				}

				// entry 路径逃逸检查
				auto entry_path = (plugin_dir / manifest.entry).lexically_normal();
				auto dir_str	= plugin_dir.lexically_normal().generic_string();
				if (entry_path.generic_string().compare(0, dir_str.size(), dir_str) != 0) {
					error_out = "entry escapes plugin directory: " + manifest.entry;
					return std::nullopt;
				}

				// 宿主版本比对
				if (!manifest.min_app_version.empty()) {
					const std::string app_version = std::to_string(GDownload_VERSION_MAJOR) + "."
													+ std::to_string(GDownload_VERSION_MINOR) + "."
													+ std::to_string(GDownload_VERSION_BUILD);
					if (CompareSemver(app_version, manifest.min_app_version) < 0) {
						error_out = "requires app version >= " + manifest.min_app_version + " (current " + app_version
									+ ")";
						return std::nullopt;
					}
				}

				return manifest;
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

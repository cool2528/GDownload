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

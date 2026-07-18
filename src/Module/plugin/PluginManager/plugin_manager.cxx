#include "plugin_manager.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include "JsPluginRuntime/js_plugin_host.h"
#include "JsPluginRuntime/plugin_manifest.h"
#include "logger.h"
namespace gdl {
	namespace plugin {

		DownloadPluginManager::~DownloadPluginManager() {
			js_plugins_.clear();
		}

		namespace {
			// 读取 data_dir/plugin_state.json 中被禁用的插件名（市场"禁用"开关写入）
			std::vector<std::string> ReadDisabledPlugins(const std::string& data_dir) {
				std::vector<std::string> disabled;
				std::ifstream f(std::filesystem::path(data_dir) / "plugin_state.json");
				if (!f.is_open()) {
					return disabled;
				}
				try {
					nlohmann::json json;
					f >> json;
					if (json.contains("disabled") && json["disabled"].is_array()) {
						disabled = json["disabled"].get<std::vector<std::string>>();
					}
				} catch (const nlohmann::json::exception&) {
				}
				return disabled;
			}
		}  // namespace

		bool DownloadPluginManager::ReloadJsPlugins(const std::string& plugins_dir, const std::string& data_dir) {
			{
				std::lock_guard<std::mutex> lock(mutex_);
				js_plugins_.clear();
			}
			return LoadJsPlugins(plugins_dir, data_dir);
		}

		bool DownloadPluginManager::LoadJsPlugins(const std::string& plugins_dir, const std::string& data_dir) {
			std::lock_guard<std::mutex> lock(mutex_);
			std::error_code ec;
			if (!std::filesystem::exists(plugins_dir, ec)) {
				LOG_INFO("js plugins dir not found: {}", plugins_dir);
				return false;
			}
			auto disabled		= ReadDisabledPlugins(data_dir);
			auto is_disabled	= [&](const std::string& name) {
				return std::find(disabled.begin(), disabled.end(), name) != disabled.end();
			};
			bool any_loaded = false;
			try {
				for (const auto& entry : std::filesystem::directory_iterator(plugins_dir)) {
					if (!entry.is_directory()) {
						continue;
					}
					std::string error;
					auto manifest = js::LoadManifest(entry.path(), error);
					if (!manifest) {
						// 无 manifest.json 的目录静默跳过，其他校验失败要打日志
						if (error != "manifest.json not found") {
							LOG_WARN("js plugin rejected: {} ({})", entry.path().string(), error);
						}
						continue;
					}
					// 被市场禁用的插件不加载
					if (is_disabled(manifest->name)) {
						LOG_INFO("js plugin disabled, skipped: {}", manifest->name);
						continue;
					}
					// 同名插件去重（先加载者优先）
					bool duplicated = std::any_of(js_plugins_.begin(), js_plugins_.end(),
												  [&](const INetDiskDownloadPlugin::IDownloadPluginPtr& p) {
													  return p->GetPluginMetadata().name == manifest->name;
												  });
					if (duplicated) {
						LOG_WARN("js plugin duplicated, skipped: {}", manifest->name);
						continue;
					}
					auto host = std::make_shared<js::JsPluginHost>(std::move(*manifest), data_dir);
					js_plugins_.push_back(host);
					any_loaded = true;
					LOG_INFO("js plugin registered: {}", entry.path().string());
				}
			} catch (const std::exception& e) {
				LOG_ERR("iterate js plugins dir failed: {}", e.what());
				return false;
			}
			return any_loaded;
		}

        std::vector<INetDiskDownloadPlugin::IDownloadPluginPtr> DownloadPluginManager::GetPluginsForUrl(
            std::string_view url) {
			std::vector<INetDiskDownloadPlugin::IDownloadPluginPtr> result;
			std::lock_guard<std::mutex> lock(mutex_);
			// string_view::data() 不保证 null 终止，CanHandle 接收 const std::string&，
			// 显式构造避免越界读取。
			const std::string url_str(url);
			for (const auto& js_plugin : js_plugins_) {
				if (js_plugin->CanHandle(url_str)) {
					result.push_back(js_plugin);
				}
			}
			return result;
		}

		INetDiskDownloadPlugin::IDownloadPluginPtr DownloadPluginManager::GetPluginByName(std::string_view name) {
			std::lock_guard<std::mutex> lock(mutex_);
			for (const auto& js_plugin : js_plugins_) {
				if (js_plugin->GetPluginMetadata().name == name) {
					return js_plugin;
				}
			}
			return nullptr;
		}

		std::optional<js::PluginManifest> DownloadPluginManager::GetManifestByName(std::string_view name) {
			std::lock_guard<std::mutex> lock(mutex_);
			for (const auto& js_plugin : js_plugins_) {
				auto host = std::dynamic_pointer_cast<js::JsPluginHost>(js_plugin);
				if (host && host->manifest().name == name) {
					return host->manifest();
				}
			}
			return std::nullopt;
		}

		DownloadPluginManager::DownloadPluginManager() {}

	}  // namespace plugin
}  // namespace gdl

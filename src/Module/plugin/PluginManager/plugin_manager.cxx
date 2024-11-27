#include "plugin_manager.h"
#include <filesystem>
#include "logger.h"
namespace gdl {
	namespace plugin {

		DownloadPluginManager::~DownloadPluginManager() {
			for (const auto& info : plugins_) {
				info.destroy_plugin(info.plugin.get());
				info.loader.UnLoad();
			}
			plugins_.clear();
		}

		bool DownloadPluginManager::LoadPlugins(const std::string& plugins_dir, const LoadPluginOptions& options) {
			std::lock_guard<std::mutex> lock(mutex_);
			for (const auto& entry : std::filesystem::directory_iterator(plugins_dir)) {
				if (entry.path().extension() == ".so" || entry.path().extension() == ".dylib" ||
					entry.path().extension() == ".dll") {
					if (!LoadPlugin(entry.path().string())) {
						LOG_WARN("loader plugin faild {}", entry.path().string());
					}
				}
			}
			return true;
		}

		bool DownloadPluginManager::LoadPlugin(const std::string& plugin_path) {
			std::error_code ec;
			if (!std::filesystem::exists(plugin_path, ec)) return false;
			loader::PluginLoader plugin_loader;
			if (!plugin_loader.Load(plugin_path)) return false;
			PluginInfo info;
			info.create_plugin	= (CreatePluginFunc)plugin_loader.GetSymbol("CreatePlugin");
			info.destroy_plugin = (DestroyPluginFunc)plugin_loader.GetSymbol("DestroyPlugin");
			if (!info.create_plugin || !info.destroy_plugin) {
				plugin_loader.UnLoad();
				return false;
			}
			IDownloadPlugin* plugin_ptr = info.create_plugin();
			if (!plugin_ptr) {
				plugin_loader.UnLoad();
				return false;
			}
			info.plugin = IDownloadPlugin::IDownloadPluginPtr(plugin_ptr);
			info.loader = plugin_loader;
			plugins_.push_back(info);
			return true;
		}

		std::vector<IDownloadPlugin::IDownloadPluginPtr> DownloadPluginManager::GetPluginsForUrl(std::string_view url) {
			std::vector<IDownloadPlugin::IDownloadPluginPtr> result;
			std::lock_guard<std::mutex> lock(mutex_);
			for (const auto& info : plugins_) {
				if (info.plugin->CanHandle(url.data())) {
					result.push_back(info.plugin);
				}
			}
			return result;
		}

		IDownloadPlugin::IDownloadPluginPtr DownloadPluginManager::GetPluginByName(std::string_view name) {
			std::lock_guard<std::mutex> lock(mutex_);
			for (const auto& info : plugins_) {
				if (info.plugin->GetPluginMetadata().name == name) {
					return info.plugin;
				}
			}
			return nullptr;
		}

		DownloadPluginManager::DownloadPluginManager() {}

	}  // namespace plugin
}  // namespace gdl

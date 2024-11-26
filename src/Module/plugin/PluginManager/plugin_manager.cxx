#include "plugin_manager.h"

namespace gdl {
	namespace plugin {

		DownloadPluginManager::~DownloadPluginManager() {}

		bool DownloadPluginManager::LoadPlugins(const std::string& plugins_dir, const LoadPluginOptions& options) {
			return false;
		}

		bool DownloadPluginManager::LoadPlugin(const std::string& pluginPath) {
			return false;
		}

		std::vector<IDownloadPlugin::IDownloadPluginPtr> DownloadPluginManager::GetPluginsForUrl(std::string_view url) {
			return {};
		}

		IDownloadPlugin::IDownloadPluginPtr DownloadPluginManager::GetPluginByName(std::string_view name) {
			return nullptr;
		}

		DownloadPluginManager::DownloadPluginManager() {}

	}  // namespace plugin
}  // namespace gdl

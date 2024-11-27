#pragma once
#include <string>
#include <vector>
#include "IDownload_Plugin.h"
#include "PluginManager_export.h"
#include "loader/plugin_loader.h"
#include "singleton.hpp"
namespace gdl {
	namespace plugin {
		class PluginManager_API DownloadPluginManager : public Singleton<DownloadPluginManager> {
		   public:
			~DownloadPluginManager() override;
			struct LoadPluginOptions {
				bool validate_signature;
				std::vector<std::string> allowed_plugins_hash_list;
				std::vector<std::string> blocked_plugins_hash_list;
			};

			bool LoadPlugins(const std::string& plugins_dir, const LoadPluginOptions& options = {});

			std::vector<IDownloadPlugin::IDownloadPluginPtr> GetPluginsForUrl(std::string_view url);
			IDownloadPlugin::IDownloadPluginPtr GetPluginByName(std::string_view name);

		   private:
			bool LoadPlugin(const std::string& plugin_path);
			explicit DownloadPluginManager();
			struct PluginInfo {
				loader::PluginLoader loader;
				IDownloadPlugin::IDownloadPluginPtr plugin;
				CreatePluginFunc create_plugin;
				DestroyPluginFunc destroy_plugin;
			};

		   private:
			std::vector<PluginInfo> plugins_;
			std::mutex mutex_;
		};
	}  // namespace plugin
}  // namespace gdl

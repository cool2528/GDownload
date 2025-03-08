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
			SINGLETON_DECLARE(DownloadPluginManager)
		   public:
			~DownloadPluginManager() override;
			struct LoadPluginOptions {
				bool validate_signature;
				std::vector<std::string> allowed_plugins_hash_list;
				std::vector<std::string> blocked_plugins_hash_list;
			};

			bool LoadPlugins(const std::string& plugins_dir, const LoadPluginOptions& options = {});

            std::vector<INetDiskDownloadPlugin::IDownloadPluginPtr> GetPluginsForUrl(std::string_view url);
            INetDiskDownloadPlugin::IDownloadPluginPtr GetPluginByName(std::string_view name);

		   private:
			bool LoadPlugin(const std::string& plugin_path);
			explicit DownloadPluginManager();

            // 插件资源管理类 用于安全加载和卸载插件
            class PluginResourceGuard {
               public:
                using Ptr = std::shared_ptr<PluginResourceGuard>;

               public:
                explicit PluginResourceGuard(const std::string& path);
                ~PluginResourceGuard();
                INetDiskDownloadPlugin::IDownloadPluginPtr GetPlugin() const { return plugin_; }
                bool InitPlugin();

               private:
                loader::PluginLoader loader_;
                INetDiskDownloadPlugin::IDownloadPluginPtr plugin_;
                CreatePluginFunc create_plugin_;
                DestroyPluginFunc destroy_plugin_;
                std::string path_;
			};

		   private:
            std::vector<PluginResourceGuard::Ptr> plugins_;
			std::mutex mutex_;
		};
	}  // namespace plugin
}  // namespace gdl

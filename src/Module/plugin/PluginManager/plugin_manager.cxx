#include "plugin_manager.h"
#include <filesystem>
#include "logger.h"
namespace gdl {
	namespace plugin {

		DownloadPluginManager::~DownloadPluginManager() {
			plugins_.clear();
		}

		bool DownloadPluginManager::LoadPlugins(const std::string& plugins_dir, const LoadPluginOptions& options) {
			std::lock_guard<std::mutex> lock(mutex_);
			for (const auto& entry : std::filesystem::directory_iterator(plugins_dir)) {
				if (entry.path().extension() == ".so" || entry.path().extension() == ".dylib" ||
					entry.path().extension() == ".dll") {
                    const auto name = entry.path().filename().string();
                    if (name.find("Plugin") == std::string::npos) {
                        continue;
                    }
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
            auto guard_plugin = std::make_shared<PluginResourceGuard>(plugin_path);
            if (!guard_plugin) {
                return false;
            }

            if (!guard_plugin->InitPlugin()) {
                return false;
            }
            plugins_.push_back(guard_plugin);
			return true;
		}

        std::vector<INetDiskDownloadPlugin::IDownloadPluginPtr> DownloadPluginManager::GetPluginsForUrl(
            std::string_view url) {
			std::vector<INetDiskDownloadPlugin::IDownloadPluginPtr> result;
			std::lock_guard<std::mutex> lock(mutex_);
			for (const auto& info : plugins_) {
                if (info->GetPlugin()->CanHandle(url.data())) {
                    result.push_back(info->GetPlugin());
				}
			}
			return result;
		}

		INetDiskDownloadPlugin::IDownloadPluginPtr DownloadPluginManager::GetPluginByName(std::string_view name) {
			std::lock_guard<std::mutex> lock(mutex_);
			for (const auto& info : plugins_) {
                if (info->GetPlugin()->GetPluginMetadata().name == name) {
                    return info->GetPlugin();
				}
			}
			return nullptr;
		}

		DownloadPluginManager::DownloadPluginManager() {}

        DownloadPluginManager::PluginResourceGuard::PluginResourceGuard(const std::string& path):path_(path) {}

        bool DownloadPluginManager::PluginResourceGuard::InitPlugin() {
            if (!loader_.Load(path_)) return false;
			create_plugin_	= reinterpret_cast<CreatePluginFunc>(loader_.GetSymbol("CreatePlugin"));
			destroy_plugin_ = reinterpret_cast<DestroyPluginFunc>(loader_.GetSymbol("DestroyPlugin"));
            if (!create_plugin_ || !destroy_plugin_) {
                loader_.UnLoad();
                return false;
            }
			plugin_ = std::shared_ptr<INetDiskDownloadPlugin>(create_plugin_(), [this](INetDiskDownloadPlugin* p) {
				if (p && destroy_plugin_) {
					destroy_plugin_(p);
					loader_.UnLoad();
                }
            });
            if (!plugin_) {
                loader_.UnLoad();
                return false;
            }
            return true;
        }

	}  // namespace plugin
}  // namespace gdl

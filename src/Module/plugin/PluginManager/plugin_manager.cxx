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
			std::error_code ec;
			if (!std::filesystem::exists(plugins_dir, ec)) return false;
			bool any_loaded = false;
			try {
				for (const auto& entry : std::filesystem::directory_iterator(plugins_dir)) {
					if (entry.path().extension() == ".so" || entry.path().extension() == ".dylib" ||
						entry.path().extension() == ".dll") {
						const auto name = entry.path().filename().string();
						if (name.find("Plugin") == std::string::npos) {
							continue;
						}
						if (LoadPlugin(entry.path().string())) {
							any_loaded = true;
						} else {
							LOG_WARN("loader plugin faild {}", entry.path().string());
						}
					}
				}
			} catch (const std::exception& e) {
				LOG_ERR("iterate plugins dir failed: {}", e.what());
				return false;
			}
			return any_loaded;
		}

		bool DownloadPluginManager::LoadPlugin(const std::string& plugin_path) {
			std::error_code ec;
			if (!std::filesystem::exists(plugin_path, ec)) return false;
			auto guard_plugin = std::make_shared<PluginResourceGuard>(plugin_path);

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
			// string_view::data() 不保证 null 终止，CanHandle 接收 const std::string&，
			// 显式构造避免越界读取。
			const std::string url_str(url);
			for (const auto& info : plugins_) {
				if (info->GetPlugin()->CanHandle(url_str)) {
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
            loader_ = std::make_shared<loader::PluginLoader>();
            if (!loader_->Load(path_)) return false;
			create_plugin_	= reinterpret_cast<CreatePluginFunc>(loader_->GetSymbol("CreatePlugin"));
			destroy_plugin_ = reinterpret_cast<DestroyPluginFunc>(loader_->GetSymbol("DestroyPlugin"));
            if (!create_plugin_ || !destroy_plugin_) {
                loader_->UnLoad();
                loader_.reset();
                return false;
            }
            // deleter 捕获 loader 与 destroy_fn 的副本，不依赖 guard 生命周期。
            // guard 析构后外部仍持有的 plugin_ 释放时，deleter 仍可安全执行：
            // 先销毁插件实例，再随 loader 副本释放自动 UnLoad（RAII），避免 UAF。
            auto loader = loader_;
            auto destroy_fn = destroy_plugin_;
			plugin_ = INetDiskDownloadPlugin::IDownloadPluginPtr(
                create_plugin_(),
                [loader, destroy_fn](INetDiskDownloadPlugin* p) {
                    if (p && destroy_fn) {
                        destroy_fn(p);
                    }
                    // loader 副本在此处释放，触发 PluginLoader 析构自动卸载动态库
                });
            if (!plugin_) {
                loader_->UnLoad();
                loader_.reset();
                return false;
            }
            return true;
        }

	}  // namespace plugin
}  // namespace gdl

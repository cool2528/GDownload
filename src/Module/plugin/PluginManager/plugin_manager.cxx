#include "plugin_manager.h"
#include <filesystem>
#include "logger.h"
#include <algorithm>
#include <fstream>
#include <openssl/evp.h>
namespace gdl {
	namespace plugin {

		namespace {
			// 计算文件 SHA-256（OpenSSL EVP），返回小写十六进制串；失败返回空串（S1）
			std::string ComputeFileSha256(const std::string& path) {
				std::ifstream file(path, std::ios::binary);
				if (!file) return {};
				EVP_MD_CTX* ctx = EVP_MD_CTX_new();
				if (!ctx) return {};
				std::string result;
				if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1) {
					char buf[8192];
					bool ok = true;
					while (file) {
						file.read(buf, sizeof(buf));
						std::streamsize n = file.gcount();
						if (n > 0 && EVP_DigestUpdate(ctx, buf, static_cast<size_t>(n)) != 1) {
							ok = false;
							break;
						}
					}
					if (ok) {
						unsigned char digest[EVP_MAX_MD_SIZE];
						unsigned int len = 0;
						if (EVP_DigestFinal_ex(ctx, digest, &len) == 1) {
							static const char* kHex = "0123456789abcdef";
							result.reserve(static_cast<size_t>(len) * 2);
							for (unsigned int i = 0; i < len; ++i) {
								result.push_back(kHex[digest[i] >> 4]);
								result.push_back(kHex[digest[i] & 0x0F]);
							}
						}
					}
				}
				EVP_MD_CTX_free(ctx);
				return result;
			}

			bool ContainsHash(const std::vector<std::string>& list, const std::string& hash) {
				return std::find(list.begin(), list.end(), hash) != list.end();
			}
		}  // namespace

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
						if (LoadPlugin(entry.path().string(), options)) {
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

		bool DownloadPluginManager::LoadPlugin(const std::string& plugin_path, const LoadPluginOptions& options) {
			std::error_code ec;
			if (!std::filesystem::exists(plugin_path, ec)) return false;

			// 计算文件 SHA-256 做黑/白名单校验（S1）；默认 validate_signature=false、黑名单为空则行为不变
			const std::string hash = ComputeFileSha256(plugin_path);
			if (hash.empty()) {
				LOG_WARN("compute plugin sha256 failed, skip: {}", plugin_path);
				return false;
			}
			if (ContainsHash(options.blocked_plugins_hash_list, hash)) {
				LOG_WARN("plugin blocked by hash blacklist: {} ({})", plugin_path, hash);
				return false;
			}
			if (options.validate_signature && !ContainsHash(options.allowed_plugins_hash_list, hash)) {
				LOG_WARN("plugin not in allowed hash list, rejected: {} ({})", plugin_path, hash);
				return false;
			}

			auto guard_plugin = std::make_shared<PluginResourceGuard>(plugin_path);
			if (!guard_plugin->InitPlugin()) {
				return false;
			}
			plugins_.push_back(guard_plugin);
			LOG_INFO("plugin loaded: {} (sha256={})", plugin_path, hash);
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

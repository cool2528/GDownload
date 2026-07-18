#pragma once
#include <optional>
#include <string>
#include <vector>
#include "IDownload_Plugin.h"
#include "JsPluginRuntime/plugin_manifest.h"
#include "PluginManager_export.h"
#include "singleton.hpp"
namespace gdl {
	namespace plugin {
		class PluginManager_API DownloadPluginManager : public Singleton<DownloadPluginManager> {
			SINGLETON_DECLARE(DownloadPluginManager)
		   public:
			~DownloadPluginManager() override;

			// 加载 JS 脚本插件：扫描 plugins_dir 下含 manifest.json 的子目录
			// data_dir 为插件数据根目录（存放 plugin_storage/ 与 plugin_cookies/）
			bool LoadJsPlugins(const std::string& plugins_dir, const std::string& data_dir);

			// 热重载：清空现有 JS 插件后重新扫描加载（市场安装/卸载/启用禁用后调用）
			bool ReloadJsPlugins(const std::string& plugins_dir, const std::string& data_dir);

            std::vector<INetDiskDownloadPlugin::IDownloadPluginPtr> GetPluginsForUrl(std::string_view url);
            INetDiskDownloadPlugin::IDownloadPluginPtr GetPluginByName(std::string_view name);

			// 取指定插件的 manifest 副本（本地化名称/声明式配置 Schema 供 UI 使用）
			std::optional<js::PluginManifest> GetManifestByName(std::string_view name);

		   private:
			explicit DownloadPluginManager();

		   private:
            // JS 脚本插件列表（JsPluginHost 实例）
            std::vector<INetDiskDownloadPlugin::IDownloadPluginPtr> js_plugins_;
			std::mutex mutex_;
		};
	}  // namespace plugin
}  // namespace gdl

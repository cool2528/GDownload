#pragma once
#include <filesystem>
#include <memory>
#include <mutex>

#include "../IDownload_Plugin.h"
#include "../plugin_config_store.h"
#include "api/api_context.h"
#include "cookie_jar.h"
#include "js_runtime.h"
#include "plugin_manifest.h"
#include "url_pattern.h"

namespace gdl {
	namespace plugin {
		namespace js {

			// JS 插件宿主：将一个 JS 插件适配为 INetDiskDownloadPlugin
			// 懒初始化：首次实际调用时才创建 JSRuntime 并加载入口模块
			// 线程模型：内部互斥锁串行化所有 JS 调用
			class JsPluginHost : public INetDiskDownloadPlugin {
			   public:
				// manifest: 已通过校验的插件清单
				// data_root: 插件数据根目录（存放 plugin_storage/ 与 plugin_cookies/）
				JsPluginHost(PluginManifest manifest, std::filesystem::path data_root);
				~JsPluginHost() override;

				// ---- INetDiskDownloadPlugin ----
				std::optional<std::vector<FileInfo>> ParseUrl(std::string_view url,
															  std::string_view user_token = "") override;
				std::optional<std::vector<FileInfo>> EnterDirectory(const FileInfo& info) override;
				std::optional<std::vector<ParseResult>> GetDownloadInfo(const FileInfo& info) override;
				PluginMetadata GetPluginMetadata() override;
				bool CanHandle(const std::string& url) const override;

			   private:
				// 懒初始化 Runtime、注入 gdl.*、加载入口模块（需持锁调用）
				bool EnsureInitializedLocked();
				// 调用插件对象上的方法并等待结果；错误时记日志并通知
				std::optional<JSValue> InvokeMethodLocked(const char* method, int argc, JSValue* argv);
				// 统一的错误上报（日志 + notify 回调）
				void ReportError(const std::string& stage);

			   private:
				PluginManifest manifest_;
				UrlPatternSet patterns_;
				std::filesystem::path data_root_;

				std::unique_ptr<JsRuntime> runtime_;
				std::unique_ptr<CookieJar> cookie_jar_;
				std::unique_ptr<PluginConfigStore> config_store_;
				ApiContext api_context_;
				// 插件的 default export 对象
				JSValue plugin_object_{JS_UNDEFINED};
				bool init_attempted_{false};
				bool init_ok_{false};
				mutable std::mutex mutex_;
			};

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

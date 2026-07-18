#pragma once
#include <filesystem>

#include "../../IDownload_Plugin.h"
#include "../../plugin_config_store.h"
#include "../cookie_jar.h"
#include "../js_runtime.h"
#include "../plugin_manifest.h"

namespace gdl {
	namespace plugin {
		namespace js {

			// 宿主 API 的共享上下文，通过 JS_SetContextOpaque 挂到 JSContext
			// 生命周期由 JsPluginHost 保证覆盖整个 Context 存活期
			struct ApiContext {
				const PluginManifest* manifest{nullptr};
				JsRuntime* runtime{nullptr};
				CookieJar* cookie_jar{nullptr};
				// 键值存储文件：plugin_storage/<name>.json
				std::filesystem::path storage_file;
				// 用户配置存储（gdl.config 只读来源；由 JsPluginHost 持有）
				PluginConfigStore* config_store{nullptr};
				// 宿主回调（指向 JsPluginHost 持有的 std::function，可能为空函数）
				const INetDiskDownloadPlugin::VerificationCallback* verification_callback{nullptr};
				const INetDiskDownloadPlugin::MessageNotifyCallback* notify_callback{nullptr};
			};

			// 从 JSContext 取回 ApiContext
			inline ApiContext* GetApiContext(JSContext* ctx) {
				return static_cast<ApiContext*>(JS_GetContextOpaque(ctx));
			}

			// 向 Context 注入全局对象 gdl（http/crypto/utils/storage/ui/log/notify）
			// 调用前需先 JS_SetContextOpaque(ctx, api_context)
			void RegisterGdlApis(JSContext* ctx);

			// 各分模块的对象构造（api_module.cxx 内部调用）
			JSValue CreateHttpApi(JSContext* ctx);
			JSValue CreateCryptoApi(JSContext* ctx);
			JSValue CreateUtilsApi(JSContext* ctx);
			JSValue CreateStorageApi(JSContext* ctx);
			JSValue CreateUiApi(JSContext* ctx);
			JSValue CreateLogApi(JSContext* ctx);
			JSValue CreateConfigApi(JSContext* ctx);
			// gdl.notify 挂在 gdl 根上（api_ui.cxx 提供实现）
			JSValue CreateNotifyFunction(JSContext* ctx);

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

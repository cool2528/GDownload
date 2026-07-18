#include "js_plugin_host.h"

#include <spdlog/spdlog.h>

#include "js_value_convert.h"

namespace gdl {
	namespace plugin {
		namespace js {

			JsPluginHost::JsPluginHost(PluginManifest manifest, std::filesystem::path data_root)
				: manifest_(std::move(manifest)), patterns_(manifest_.url_patterns), data_root_(std::move(data_root)) {}

			JsPluginHost::~JsPluginHost() {
				std::lock_guard<std::mutex> lock(mutex_);
				if (runtime_ && runtime_->context() && !JS_IsUndefined(plugin_object_)) {
					JS_FreeValue(runtime_->context(), plugin_object_);
				}
				// runtime_ 在 cookie_jar_ 之前析构（Context 引用了 ApiContext 中的 jar 指针）
				runtime_.reset();
				config_store_.reset();
				cookie_jar_.reset();
			}

			bool JsPluginHost::CanHandle(const std::string& url) const {
				// 快速路径：manifest url_patterns 宿主侧匹配，不进入 JS 引擎（设计文档 3.1-5）
				// 插件可选的 canHandle() 二次精确判断在 ParseUrl 内部执行
				return patterns_.Matches(url);
			}

			INetDiskDownloadPlugin::PluginMetadata JsPluginHost::GetPluginMetadata() {
				// 元数据来自 manifest 直读，无需初始化 JS 运行时（设计文档 3.1-6）
				PluginMetadata metadata;
				metadata.name			 = manifest_.name;
				metadata.version		 = manifest_.version;
				metadata.author			 = manifest_.author;
				metadata.description	 = manifest_.description;
				metadata.supported_domains = manifest_.url_patterns;
				return metadata;
			}

			bool JsPluginHost::EnsureInitializedLocked() {
				if (init_attempted_) {
					return init_ok_;
				}
				init_attempted_ = true;

				JsRuntime::Options options;
				options.module_root = manifest_.plugin_dir;
				runtime_			= std::make_unique<JsRuntime>(options);
				if (!runtime_->context()) {
					spdlog::error("[js-plugin:{}] runtime creation failed", manifest_.name);
					return false;
				}

				cookie_jar_ = std::make_unique<CookieJar>(data_root_ / "plugin_cookies" / (manifest_.name + ".json"),
														  manifest_.permissions.http_domains);
				cookie_jar_->Load();

				// 用户配置存储：gdl.config 只读来源
				config_store_			 = std::make_unique<PluginConfigStore>(data_root_);
				api_context_.config_store = config_store_.get();

				// 组装 ApiContext 并注入 gdl.*
				api_context_.manifest			   = &manifest_;
				api_context_.runtime			   = runtime_.get();
				api_context_.cookie_jar			   = cookie_jar_.get();
				api_context_.storage_file		   = data_root_ / "plugin_storage" / (manifest_.name + ".json");
				api_context_.verification_callback = &verification_callback_;
				api_context_.notify_callback	   = &message_notify_callback_;
				JS_SetContextOpaque(runtime_->context(), &api_context_);
				RegisterGdlApis(runtime_->context());

				// 加载入口模块
				auto default_export = runtime_->LoadModuleDefaultExport(manifest_.EntryPath());
				if (!default_export) {
					spdlog::error("[js-plugin:{}] load entry failed: {}\n{}", manifest_.name,
								  runtime_->last_error().message, runtime_->last_error().stack);
					return false;
				}
				plugin_object_ = *default_export;
				init_ok_	   = true;
				spdlog::info("[js-plugin:{}] loaded (version {})", manifest_.name, manifest_.version);
				return true;
			}

			void JsPluginHost::ReportError(const std::string& stage) {
				const auto& error = runtime_->last_error();
				spdlog::error("[js-plugin:{}] {} failed: {}\n{}", manifest_.name, stage, error.message, error.stack);
				if (message_notify_callback_) {
					auto message = manifest_.display_name + ": " + stage + " failed: " + error.message;
					message_notify_callback_(message, error.timeout ? MsgType::kWarning : MsgType::kError);
				}
			}

			std::optional<JSValue> JsPluginHost::InvokeMethodLocked(const char* method, int argc, JSValue* argv) {
				auto* ctx = runtime_->context();
				JsValueGuard method_fn(ctx, JS_GetPropertyStr(ctx, plugin_object_, method));
				if (!JS_IsFunction(ctx, method_fn.get())) {
					spdlog::warn("[js-plugin:{}] method '{}' not implemented", manifest_.name, method);
					return std::nullopt;
				}
				return runtime_->CallAndAwait(method_fn.get(), plugin_object_, argc, argv);
			}

			std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> JsPluginHost::ParseUrl(
				std::string_view url, std::string_view user_token) {
				std::lock_guard<std::mutex> lock(mutex_);
				if (!EnsureInitializedLocked()) {
					return std::nullopt;
				}
				auto* ctx = runtime_->context();

				// 可选的 canHandle 二次精确判断
				{
					JsValueGuard can_handle_fn(ctx, JS_GetPropertyStr(ctx, plugin_object_, "canHandle"));
					if (JS_IsFunction(ctx, can_handle_fn.get())) {
						JSValue url_arg = JS_NewStringLen(ctx, url.data(), url.size());
						auto can_handle = runtime_->CallAndAwait(can_handle_fn.get(), plugin_object_, 1, &url_arg);
						JS_FreeValue(ctx, url_arg);
						if (can_handle) {
							bool accepted = JS_ToBool(ctx, *can_handle) != 0;
							JS_FreeValue(ctx, *can_handle);
							if (!accepted) {
								return std::nullopt;
							}
						}
					}
				}

				JSValue args[2] = {JS_NewStringLen(ctx, url.data(), url.size()),
								   JS_NewStringLen(ctx, user_token.data(), user_token.size())};
				auto result		= InvokeMethodLocked("parseUrl", 2, args);
				JS_FreeValue(ctx, args[0]);
				JS_FreeValue(ctx, args[1]);
				if (!result) {
					ReportError("parseUrl");
					return std::nullopt;
				}
				JsValueGuard result_guard(ctx, *result);
				return JsToFileInfoVector(ctx, result_guard.get());
			}

			std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> JsPluginHost::EnterDirectory(
				const FileInfo& info) {
				std::lock_guard<std::mutex> lock(mutex_);
				if (!EnsureInitializedLocked()) {
					return std::nullopt;
				}
				auto* ctx	= runtime_->context();
				JSValue arg = FileInfoToJs(ctx, info);
				auto result = InvokeMethodLocked("enterDirectory", 1, &arg);
				JS_FreeValue(ctx, arg);
				if (!result) {
					ReportError("enterDirectory");
					return std::nullopt;
				}
				JsValueGuard result_guard(ctx, *result);
				return JsToFileInfoVector(ctx, result_guard.get());
			}

			std::optional<std::vector<INetDiskDownloadPlugin::ParseResult>> JsPluginHost::GetDownloadInfo(
				const FileInfo& info) {
				std::lock_guard<std::mutex> lock(mutex_);
				if (!EnsureInitializedLocked()) {
					return std::nullopt;
				}
				auto* ctx	= runtime_->context();
				JSValue arg = FileInfoToJs(ctx, info);
				auto result = InvokeMethodLocked("getDownloadInfo", 1, &arg);
				JS_FreeValue(ctx, arg);
				if (!result) {
					ReportError("getDownloadInfo");
					return std::nullopt;
				}
				JsValueGuard result_guard(ctx, *result);
				return JsToParseResultVector(ctx, result_guard.get());
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

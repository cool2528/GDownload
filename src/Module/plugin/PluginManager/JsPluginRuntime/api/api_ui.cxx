#include <spdlog/spdlog.h>

#include <chrono>
#include <string>

#include "api_context.h"

namespace gdl {
	namespace plugin {
		namespace js {

			namespace {
				// 验证码交互给予的额外时间预算（用户输入可能很慢）
				constexpr auto kVerificationExtraTime = std::chrono::minutes(5);

				std::string GetOptionalStringProp(JSContext* ctx, JSValueConst obj, const char* prop) {
					JSValue value = JS_GetPropertyStr(ctx, obj, prop);
					std::string result;
					if (JS_IsString(value)) {
						const char* str = JS_ToCString(ctx, value);
						if (str) {
							result = str;
							JS_FreeCString(ctx, str);
						}
					}
					JS_FreeValue(ctx, value);
					return result;
				}

				// gdl.ui.requestVerification({imageBase64?, message?}) → string
				// 桥接现有 VerificationCallback：同步阻塞等待 UI 侧填充 input_result
				JSValue RequestVerificationFn(JSContext* ctx, JSValueConst /*this_val*/, int argc,
											  JSValueConst* argv) {
					auto* api = GetApiContext(ctx);
					if (!api || !api->manifest) {
						return JS_ThrowInternalError(ctx, "api context missing");
					}
					if (!api->manifest->permissions.verification_ui) {
						return JS_ThrowTypeError(ctx, "verification_ui permission not granted");
					}
					if (!api->verification_callback || !*api->verification_callback) {
						return JS_ThrowInternalError(ctx, "verification ui unavailable");
					}

					INetDiskDownloadPlugin::VerificationCallbackParam param;
					if (argc >= 1 && JS_IsObject(argv[0])) {
						param.image_base64 = GetOptionalStringProp(ctx, argv[0], "imageBase64");
						param.message	   = GetOptionalStringProp(ctx, argv[0], "message");
					}

					// 用户输入耗时不计入插件执行超时
					if (api->runtime) {
						api->runtime->ExtendDeadline(
							std::chrono::duration_cast<std::chrono::milliseconds>(kVerificationExtraTime));
					}
					(*api->verification_callback)(param);

					// 空输入视为用户取消
					if (param.input_result.empty()) {
						return JS_ThrowPlainError(ctx, "verification cancelled by user");
					}
					return JS_NewStringLen(ctx, param.input_result.c_str(), param.input_result.size());
				}

				// gdl.notify(message, level)：桥接 MessageNotifyCallback
				JSValue NotifyFn(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv) {
					auto* api = GetApiContext(ctx);
					if (!api) {
						return JS_ThrowInternalError(ctx, "api context missing");
					}
					if (argc < 1) {
						return JS_ThrowTypeError(ctx, "message is required");
					}
					const char* message = JS_ToCString(ctx, argv[0]);
					if (!message) {
						return JS_ThrowTypeError(ctx, "message must be a string");
					}
					// level 字符串映射到 MsgType，默认 info
					auto msg_type = INetDiskDownloadPlugin::MsgType::kInfo;
					if (argc >= 2 && JS_IsString(argv[1])) {
						const char* level = JS_ToCString(ctx, argv[1]);
						if (level) {
							std::string level_str(level);
							if (level_str == "success") {
								msg_type = INetDiskDownloadPlugin::MsgType::kSuccess;
							} else if (level_str == "error") {
								msg_type = INetDiskDownloadPlugin::MsgType::kError;
							} else if (level_str == "warning") {
								msg_type = INetDiskDownloadPlugin::MsgType::kWarning;
							} else if (level_str == "debug") {
								msg_type = INetDiskDownloadPlugin::MsgType::kDebug;
							}
							JS_FreeCString(ctx, level);
						}
					}
					if (api->notify_callback && *api->notify_callback) {
						(*api->notify_callback)(message, msg_type);
					} else {
						spdlog::info("[js-plugin] notify (no callback): {}", message);
					}
					JS_FreeCString(ctx, message);
					return JS_UNDEFINED;
				}
			}  // namespace

			JSValue CreateUiApi(JSContext* ctx) {
				JSValue ui = JS_NewObject(ctx);
				JS_SetPropertyStr(ctx, ui, "requestVerification",
								  JS_NewCFunction(ctx, RequestVerificationFn, "requestVerification", 1));
				return ui;
			}

			JSValue CreateNotifyFunction(JSContext* ctx) {
				return JS_NewCFunction(ctx, NotifyFn, "notify", 2);
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

#include <spdlog/spdlog.h>

#include <string>

#include "api_context.h"

namespace gdl {
	namespace plugin {
		namespace js {

			namespace {
				// magic: 0=debug 1=info 2=warn 3=error
				JSValue LogFn(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv, int magic) {
					if (argc < 1) {
						return JS_UNDEFINED;
					}
					const char* message = JS_ToCString(ctx, argv[0]);
					if (!message) {
						return JS_UNDEFINED;
					}
					auto* api		 = GetApiContext(ctx);
					std::string name = api && api->manifest ? api->manifest->name : "unknown";
					switch (magic) {
						case 0:
							spdlog::debug("[js-plugin:{}] {}", name, message);
							break;
						case 1:
							spdlog::info("[js-plugin:{}] {}", name, message);
							break;
						case 2:
							spdlog::warn("[js-plugin:{}] {}", name, message);
							break;
						default:
							spdlog::error("[js-plugin:{}] {}", name, message);
							break;
					}
					JS_FreeCString(ctx, message);
					return JS_UNDEFINED;
				}
			}  // namespace

			JSValue CreateLogApi(JSContext* ctx) {
				JSValue log = JS_NewObject(ctx);
				JS_SetPropertyStr(ctx, log, "debug",
								  JS_NewCFunctionMagic(ctx, LogFn, "debug", 1, JS_CFUNC_generic_magic, 0));
				JS_SetPropertyStr(ctx, log, "info",
								  JS_NewCFunctionMagic(ctx, LogFn, "info", 1, JS_CFUNC_generic_magic, 1));
				JS_SetPropertyStr(ctx, log, "warn",
								  JS_NewCFunctionMagic(ctx, LogFn, "warn", 1, JS_CFUNC_generic_magic, 2));
				JS_SetPropertyStr(ctx, log, "error",
								  JS_NewCFunctionMagic(ctx, LogFn, "error", 1, JS_CFUNC_generic_magic, 3));
				return log;
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

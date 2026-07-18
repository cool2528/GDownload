#include "api_context.h"

namespace gdl {
	namespace plugin {
		namespace js {

			void RegisterGdlApis(JSContext* ctx) {
				JSValue global = JS_GetGlobalObject(ctx);
				JSValue gdl	   = JS_NewObject(ctx);

				JS_SetPropertyStr(ctx, gdl, "http", CreateHttpApi(ctx));
				JS_SetPropertyStr(ctx, gdl, "crypto", CreateCryptoApi(ctx));
				JS_SetPropertyStr(ctx, gdl, "utils", CreateUtilsApi(ctx));
				JS_SetPropertyStr(ctx, gdl, "storage", CreateStorageApi(ctx));
				JS_SetPropertyStr(ctx, gdl, "ui", CreateUiApi(ctx));
				JS_SetPropertyStr(ctx, gdl, "log", CreateLogApi(ctx));
				JS_SetPropertyStr(ctx, gdl, "config", CreateConfigApi(ctx));
				JS_SetPropertyStr(ctx, gdl, "notify", CreateNotifyFunction(ctx));

				JS_SetPropertyStr(ctx, global, "gdl", gdl);
				JS_FreeValue(ctx, global);
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

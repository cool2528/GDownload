#include <nlohmann/json.hpp>

#include "api_context.h"

namespace gdl {
	namespace plugin {
		namespace js {

			namespace {
				// gdl.config.get(key)：用户已存值 > schema default > null（只读，设计文档第 4 节）
				JSValue ConfigGet(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv) {
					auto* api = GetApiContext(ctx);
					if (!api || !api->manifest) {
						return JS_ThrowTypeError(ctx, "config unavailable");
					}
					if (argc < 1) {
						return JS_ThrowTypeError(ctx, "key is required");
					}
					const char* key_cstr = JS_ToCString(ctx, argv[0]);
					if (!key_cstr) {
						return JS_ThrowTypeError(ctx, "key must be a string");
					}
					std::string key(key_cstr);
					JS_FreeCString(ctx, key_cstr);

					// 用户已存值优先（每次读都透传到 store，保存后立即可见）
					if (api->config_store) {
						auto value = api->config_store->GetValue(api->manifest->name, key);
						if (value) {
							switch (value->type) {
								case ConfigValue::Type::Bool:
									return JS_NewBool(ctx, value->bool_value);
								case ConfigValue::Type::Number:
									return JS_NewFloat64(ctx, value->number_value);
								case ConfigValue::Type::String:
								default:
									return JS_NewStringLen(ctx, value->string_value.c_str(),
														   value->string_value.size());
							}
						}
					}

					// 回退 schema default
					for (const auto& field : api->manifest->settings) {
						if (field.key != key) {
							continue;
						}
						if (field.default_json.empty()) {
							break;
						}
						try {
							auto def = nlohmann::json::parse(field.default_json);
							if (def.is_boolean()) {
								return JS_NewBool(ctx, def.get<bool>());
							}
							if (def.is_number()) {
								return JS_NewFloat64(ctx, def.get<double>());
							}
							if (def.is_string()) {
								auto s = def.get<std::string>();
								return JS_NewStringLen(ctx, s.c_str(), s.size());
							}
						} catch (const nlohmann::json::exception&) {
						}
						break;
					}
					return JS_NULL;
				}
			}  // namespace

			JSValue CreateConfigApi(JSContext* ctx) {
				JSValue config = JS_NewObject(ctx);
				JS_SetPropertyStr(ctx, config, "get", JS_NewCFunction(ctx, ConfigGet, "get", 1));
				return config;
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

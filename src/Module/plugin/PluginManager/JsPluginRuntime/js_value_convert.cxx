#include "js_value_convert.h"

#include <spdlog/spdlog.h>

#include <functional>
#include <string>

namespace gdl {
	namespace plugin {
		namespace js {

			namespace {
				std::string GetString(JSContext* ctx, JSValueConst obj, const char* prop) {
					JSValue value = JS_GetPropertyStr(ctx, obj, prop);
					std::string result;
					if (!JS_IsUndefined(value) && !JS_IsNull(value) && !JS_IsException(value)) {
						const char* str = JS_ToCString(ctx, value);
						if (str) {
							result = str;
							JS_FreeCString(ctx, str);
						}
					}
					JS_FreeValue(ctx, value);
					return result;
				}

				int64_t GetInt64(JSContext* ctx, JSValueConst obj, const char* prop, int64_t default_value = 0) {
					JSValue value  = JS_GetPropertyStr(ctx, obj, prop);
					int64_t result = default_value;
					if (JS_IsNumber(value)) {
						JS_ToInt64(ctx, &result, value);
					}
					JS_FreeValue(ctx, value);
					return result;
				}

				bool GetBool(JSContext* ctx, JSValueConst obj, const char* prop, bool default_value = false) {
					JSValue value = JS_GetPropertyStr(ctx, obj, prop);
					bool result	  = default_value;
					if (JS_IsBool(value)) {
						result = JS_ToBool(ctx, value) != 0;
					}
					JS_FreeValue(ctx, value);
					return result;
				}

				// JS 对象 → multimap（数组值表达同名多值，设计文档 5.2）
				void JsObjectToMultimap(JSContext* ctx, JSValueConst obj,
										std::unordered_multimap<std::string, std::string>& map_out) {
					JSPropertyEnum* props = nullptr;
					uint32_t count		  = 0;
					if (JS_GetOwnPropertyNames(ctx, &props, &count, obj,
											   JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) != 0) {
						return;
					}
					for (uint32_t i = 0; i < count; ++i) {
						const char* key = JS_AtomToCString(ctx, props[i].atom);
						if (!key) {
							continue;
						}
						JSValue value = JS_GetProperty(ctx, obj, props[i].atom);
						if (JS_IsArray(value)) {
							JSValue length_val = JS_GetPropertyStr(ctx, value, "length");
							int64_t length	   = 0;
							JS_ToInt64(ctx, &length, length_val);
							JS_FreeValue(ctx, length_val);
							for (int64_t j = 0; j < length; ++j) {
								JSValue item		 = JS_GetPropertyUint32(ctx, value, static_cast<uint32_t>(j));
								const char* item_str = JS_ToCString(ctx, item);
								if (item_str) {
									map_out.emplace(key, item_str);
									JS_FreeCString(ctx, item_str);
								}
								JS_FreeValue(ctx, item);
							}
						} else if (!JS_IsUndefined(value) && !JS_IsNull(value)) {
							const char* value_str = JS_ToCString(ctx, value);
							if (value_str) {
								map_out.emplace(key, value_str);
								JS_FreeCString(ctx, value_str);
							}
						}
						JS_FreeValue(ctx, value);
						JS_FreeCString(ctx, key);
					}
					JS_FreePropertyEnum(ctx, props, count);
				}

				// 遍历 JS 数组，对每个元素调用 fn；返回 false 表示 value 不是数组
				bool ForEachArrayItem(JSContext* ctx, JSValueConst value,
									  const std::function<void(JSValueConst)>& fn) {
					if (!JS_IsArray(value)) {
						return false;
					}
					JSValue length_val = JS_GetPropertyStr(ctx, value, "length");
					int64_t length	   = 0;
					JS_ToInt64(ctx, &length, length_val);
					JS_FreeValue(ctx, length_val);
					for (int64_t i = 0; i < length; ++i) {
						JSValue item = JS_GetPropertyUint32(ctx, value, static_cast<uint32_t>(i));
						fn(item);
						JS_FreeValue(ctx, item);
					}
					return true;
				}
			}  // namespace

			JSValue FileInfoToJs(JSContext* ctx, const INetDiskDownloadPlugin::FileInfo& info) {
				JSValue obj = JS_NewObject(ctx);
				JS_SetPropertyStr(ctx, obj, "path", JS_NewString(ctx, info.path.c_str()));
				JS_SetPropertyStr(ctx, obj, "name", JS_NewString(ctx, info.name.c_str()));
				JS_SetPropertyStr(ctx, obj, "size", JS_NewInt64(ctx, static_cast<int64_t>(info.size)));
				JS_SetPropertyStr(ctx, obj, "is_dir", JS_NewBool(ctx, info.is_dir));
				JS_SetPropertyStr(ctx, obj, "file_id", JS_NewString(ctx, info.file_id.c_str()));
				JS_SetPropertyStr(ctx, obj, "create_time", JS_NewInt64(ctx, static_cast<int64_t>(info.create_time)));
				JS_SetPropertyStr(ctx, obj, "root_path", JS_NewString(ctx, info.root_path.c_str()));
				return obj;
			}

			std::optional<INetDiskDownloadPlugin::FileInfo> JsToFileInfo(JSContext* ctx, JSValueConst value) {
				if (!JS_IsObject(value)) {
					return std::nullopt;
				}
				INetDiskDownloadPlugin::FileInfo info;
				info.name = GetString(ctx, value, "name");
				if (info.name.empty()) {
					spdlog::warn("[js-plugin] FileInfo.name is empty, skipped");
					return std::nullopt;
				}
				info.path		 = GetString(ctx, value, "path");
				info.size		 = static_cast<size_t>(GetInt64(ctx, value, "size"));
				info.is_dir		 = GetBool(ctx, value, "is_dir");
				info.type		 = info.is_dir ? INetDiskDownloadPlugin::FileType::DIR
											   : INetDiskDownloadPlugin::FileType::FILE;
				info.file_id	 = GetString(ctx, value, "file_id");
				info.create_time = static_cast<std::uint64_t>(GetInt64(ctx, value, "create_time"));
				info.root_path	 = GetString(ctx, value, "root_path");
				return info;
			}

			std::optional<INetDiskDownloadPlugin::ParseResult> JsToParseResult(JSContext* ctx, JSValueConst value) {
				if (!JS_IsObject(value)) {
					return std::nullopt;
				}
				INetDiskDownloadPlugin::ParseResult result;
				result.real_url = GetString(ctx, value, "real_url");
				if (result.real_url.empty()) {
					spdlog::warn("[js-plugin] ParseResult.real_url is empty, skipped");
					return std::nullopt;
				}
				result.file_name = GetString(ctx, value, "file_name");
				result.file_size = static_cast<size_t>(GetInt64(ctx, value, "file_size"));

				JSValue headers = JS_GetPropertyStr(ctx, value, "headers");
				if (JS_IsObject(headers)) {
					JsObjectToMultimap(ctx, headers, result.headers);
				}
				JS_FreeValue(ctx, headers);

				JSValue options = JS_GetPropertyStr(ctx, value, "options");
				if (JS_IsObject(options)) {
					JsObjectToMultimap(ctx, options, result.options);
				}
				JS_FreeValue(ctx, options);

				JSValue mirrors = JS_GetPropertyStr(ctx, value, "mirrors");
				ForEachArrayItem(ctx, mirrors, [&](JSValueConst item) {
					const char* url = JS_ToCString(ctx, item);
					if (url) {
						result.mirrors.emplace_back(url);
						JS_FreeCString(ctx, url);
					}
				});
				JS_FreeValue(ctx, mirrors);
				return result;
			}

			std::optional<std::vector<INetDiskDownloadPlugin::FileInfo>> JsToFileInfoVector(JSContext* ctx,
																							JSValueConst value) {
				std::vector<INetDiskDownloadPlugin::FileInfo> list;
				bool is_array = ForEachArrayItem(ctx, value, [&](JSValueConst item) {
					auto info = JsToFileInfo(ctx, item);
					if (info) {
						list.push_back(std::move(*info));
					}
				});
				if (!is_array) {
					return std::nullopt;
				}
				return list;
			}

			std::optional<std::vector<INetDiskDownloadPlugin::ParseResult>> JsToParseResultVector(JSContext* ctx,
																								  JSValueConst value) {
				std::vector<INetDiskDownloadPlugin::ParseResult> list;
				bool is_array = ForEachArrayItem(ctx, value, [&](JSValueConst item) {
					auto result = JsToParseResult(ctx, item);
					if (result) {
						list.push_back(std::move(*result));
					}
				});
				if (!is_array) {
					return std::nullopt;
				}
				return list;
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

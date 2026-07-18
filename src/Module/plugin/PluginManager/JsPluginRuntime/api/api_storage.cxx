#include <spdlog/spdlog.h>

#include <fstream>
#include <nlohmann/json.hpp>

#include "api_context.h"

namespace gdl {
	namespace plugin {
		namespace js {

			namespace {
				// 单插件存储限额（设计文档 5.4）
				constexpr size_t kMaxStorageBytes = 1ull * 1024 * 1024;

				nlohmann::json LoadStorageFile(const std::filesystem::path& path) {
					std::ifstream file(path);
					if (!file.is_open()) {
						return nlohmann::json::object();
					}
					try {
						nlohmann::json json;
						file >> json;
						return json.is_object() ? json : nlohmann::json::object();
					} catch (const nlohmann::json::exception&) {
						return nlohmann::json::object();
					}
				}

				bool SaveStorageFile(const std::filesystem::path& path, const nlohmann::json& json) {
					std::error_code ec;
					std::filesystem::create_directories(path.parent_path(), ec);
					// 原子写入：临时文件 + rename
					auto tmp_path = path;
					tmp_path += ".tmp";
					{
						std::ofstream file(tmp_path, std::ios::trunc);
						if (!file.is_open()) {
							return false;
						}
						file << json.dump();
					}
					std::filesystem::rename(tmp_path, path, ec);
					return !ec;
				}

				// 权限校验（permissions.storage）
				bool CheckStoragePermission(JSContext* ctx, ApiContext** api_out) {
					auto* api = GetApiContext(ctx);
					if (!api || !api->manifest) {
						return false;
					}
					if (!api->manifest->permissions.storage) {
						return false;
					}
					*api_out = api;
					return true;
				}

				// magic: 0=get 1=set 2=remove
				JSValue StorageFn(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv, int magic) {
					ApiContext* api = nullptr;
					if (!CheckStoragePermission(ctx, &api)) {
						return JS_ThrowTypeError(ctx, "storage permission not granted");
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

					auto storage = LoadStorageFile(api->storage_file);
					switch (magic) {
						case 0: {  // get(key) → string | null
							auto it = storage.find(key);
							if (it == storage.end() || !it->is_string()) {
								return JS_NULL;
							}
							auto value = it->get<std::string>();
							return JS_NewStringLen(ctx, value.c_str(), value.size());
						}
						case 1: {  // set(key, value)
							if (argc < 2) {
								return JS_ThrowTypeError(ctx, "value is required");
							}
							const char* value_cstr = JS_ToCString(ctx, argv[1]);
							if (!value_cstr) {
								return JS_ThrowTypeError(ctx, "value must be a string");
							}
							storage[key] = value_cstr;
							JS_FreeCString(ctx, value_cstr);
							// 限额检查
							if (storage.dump().size() > kMaxStorageBytes) {
								return JS_ThrowRangeError(ctx, "storage quota exceeded (1MB)");
							}
							if (!SaveStorageFile(api->storage_file, storage)) {
								return JS_ThrowInternalError(ctx, "storage write failed");
							}
							return JS_UNDEFINED;
						}
						case 2: {  // remove(key)
							storage.erase(key);
							if (!SaveStorageFile(api->storage_file, storage)) {
								return JS_ThrowInternalError(ctx, "storage write failed");
							}
							return JS_UNDEFINED;
						}
						default:
							return JS_UNDEFINED;
					}
				}
			}  // namespace

			JSValue CreateStorageApi(JSContext* ctx) {
				JSValue storage = JS_NewObject(ctx);
				JS_SetPropertyStr(ctx, storage, "get",
								  JS_NewCFunctionMagic(ctx, StorageFn, "get", 1, JS_CFUNC_generic_magic, 0));
				JS_SetPropertyStr(ctx, storage, "set",
								  JS_NewCFunctionMagic(ctx, StorageFn, "set", 2, JS_CFUNC_generic_magic, 1));
				JS_SetPropertyStr(ctx, storage, "remove",
								  JS_NewCFunctionMagic(ctx, StorageFn, "remove", 1, JS_CFUNC_generic_magic, 2));
				return storage;
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

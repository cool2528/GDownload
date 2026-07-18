#include <openssl/evp.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "api_context.h"

namespace gdl {
	namespace plugin {
		namespace js {

			namespace {
				constexpr int64_t kMaxSleepMs = 10000;

				// gdl.utils.base64Encode / base64Decode；magic: 0=encode 1=decode
				JSValue Base64Fn(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv, int magic) {
					if (argc < 1) {
						return JS_ThrowTypeError(ctx, "input string is required");
					}
					size_t len		  = 0;
					const char* input = JS_ToCStringLen(ctx, &len, argv[0]);
					if (!input) {
						return JS_ThrowTypeError(ctx, "input must be a string");
					}
					JSValue result = JS_UNDEFINED;
					if (magic == 0) {
						// 编码输出长度：4 * ceil(n/3) + 1
						std::vector<unsigned char> encoded(((len + 2) / 3) * 4 + 1);
						int out_len = EVP_EncodeBlock(encoded.data(),
													  reinterpret_cast<const unsigned char*>(input),
													  static_cast<int>(len));
						result		= out_len >= 0 ? JS_NewStringLen(ctx, reinterpret_cast<char*>(encoded.data()),
																	 static_cast<size_t>(out_len))
												   : JS_ThrowInternalError(ctx, "base64 encode failed");
					} else {
						std::vector<unsigned char> decoded((len / 4 + 1) * 3 + 1);
						int out_len = EVP_DecodeBlock(decoded.data(),
													  reinterpret_cast<const unsigned char*>(input),
													  static_cast<int>(len));
						if (out_len < 0) {
							result = JS_ThrowTypeError(ctx, "invalid base64 input");
						} else {
							// EVP_DecodeBlock 不处理填充，按 '=' 修正实际长度
							size_t padding = 0;
							if (len >= 1 && input[len - 1] == '=') {
								++padding;
							}
							if (len >= 2 && input[len - 2] == '=') {
								++padding;
							}
							result = JS_NewStringLen(ctx, reinterpret_cast<char*>(decoded.data()),
													 static_cast<size_t>(out_len) - padding);
						}
					}
					JS_FreeCString(ctx, input);
					return result;
				}

				bool IsUrlUnreserved(unsigned char c) {
					return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'
						   || c == '_' || c == '.' || c == '~';
				}

				// gdl.utils.urlEncode / urlDecode；magic: 0=encode 1=decode
				JSValue UrlCodecFn(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv, int magic) {
					if (argc < 1) {
						return JS_ThrowTypeError(ctx, "input string is required");
					}
					size_t len		  = 0;
					const char* input = JS_ToCStringLen(ctx, &len, argv[0]);
					if (!input) {
						return JS_ThrowTypeError(ctx, "input must be a string");
					}
					std::string output;
					if (magic == 0) {
						static const char* kHexChars = "0123456789ABCDEF";
						output.reserve(len * 3);
						for (size_t i = 0; i < len; ++i) {
							auto c = static_cast<unsigned char>(input[i]);
							if (IsUrlUnreserved(c)) {
								output += static_cast<char>(c);
							} else {
								output += '%';
								output += kHexChars[c >> 4];
								output += kHexChars[c & 0x0f];
							}
						}
					} else {
						output.reserve(len);
						for (size_t i = 0; i < len; ++i) {
							if (input[i] == '%' && i + 2 < len && std::isxdigit(static_cast<unsigned char>(input[i + 1]))
								&& std::isxdigit(static_cast<unsigned char>(input[i + 2]))) {
								auto hex_value = [](char c) -> int {
									if (c >= '0' && c <= '9') {
										return c - '0';
									}
									return (std::tolower(static_cast<unsigned char>(c)) - 'a') + 10;
								};
								output += static_cast<char>(hex_value(input[i + 1]) * 16 + hex_value(input[i + 2]));
								i += 2;
							} else if (input[i] == '+') {
								output += ' ';
							} else {
								output += input[i];
							}
						}
					}
					JS_FreeCString(ctx, input);
					return JS_NewStringLen(ctx, output.c_str(), output.size());
				}

				// gdl.utils.sleep(ms)：宿主侧阻塞睡眠（上限 10s），并顺延执行超时
				JSValue SleepFn(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv) {
					int64_t ms = 0;
					if (argc >= 1) {
						JS_ToInt64(ctx, &ms, argv[0]);
					}
					ms		  = std::clamp<int64_t>(ms, 0, kMaxSleepMs);
					auto* api = GetApiContext(ctx);
					if (api && api->runtime) {
						// 睡眠时间不计入插件的执行超时预算
						api->runtime->ExtendDeadline(std::chrono::milliseconds(ms));
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(ms));
					return JS_UNDEFINED;
				}
			}  // namespace

			JSValue CreateUtilsApi(JSContext* ctx) {
				JSValue utils = JS_NewObject(ctx);
				JS_SetPropertyStr(ctx, utils, "base64Encode",
								  JS_NewCFunctionMagic(ctx, Base64Fn, "base64Encode", 1, JS_CFUNC_generic_magic, 0));
				JS_SetPropertyStr(ctx, utils, "base64Decode",
								  JS_NewCFunctionMagic(ctx, Base64Fn, "base64Decode", 1, JS_CFUNC_generic_magic, 1));
				JS_SetPropertyStr(ctx, utils, "urlEncode",
								  JS_NewCFunctionMagic(ctx, UrlCodecFn, "urlEncode", 1, JS_CFUNC_generic_magic, 0));
				JS_SetPropertyStr(ctx, utils, "urlDecode",
								  JS_NewCFunctionMagic(ctx, UrlCodecFn, "urlDecode", 1, JS_CFUNC_generic_magic, 1));
				JS_SetPropertyStr(ctx, utils, "sleep", JS_NewCFunction(ctx, SleepFn, "sleep", 1));
				return utils;
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <string>

#include "api_context.h"

namespace gdl {
	namespace plugin {
		namespace js {

			namespace {
				std::string ToHex(const unsigned char* data, size_t len) {
					static const char* kHexChars = "0123456789abcdef";
					std::string hex;
					hex.reserve(len * 2);
					for (size_t i = 0; i < len; ++i) {
						hex += kHexChars[data[i] >> 4];
						hex += kHexChars[data[i] & 0x0f];
					}
					return hex;
				}

				// 摘要计算；magic: 0=md5 1=sha1 2=sha256
				JSValue DigestFn(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv, int magic) {
					if (argc < 1) {
						return JS_ThrowTypeError(ctx, "input string is required");
					}
					size_t len		  = 0;
					const char* input = JS_ToCStringLen(ctx, &len, argv[0]);
					if (!input) {
						return JS_ThrowTypeError(ctx, "input must be a string");
					}
					const EVP_MD* md = magic == 0 ? EVP_md5() : (magic == 1 ? EVP_sha1() : EVP_sha256());
					unsigned char digest[EVP_MAX_MD_SIZE];
					unsigned int digest_len = 0;
					bool ok					= false;
					EVP_MD_CTX* md_ctx		= EVP_MD_CTX_new();
					if (md_ctx) {
						ok = EVP_DigestInit_ex(md_ctx, md, nullptr) == 1
							 && EVP_DigestUpdate(md_ctx, input, len) == 1
							 && EVP_DigestFinal_ex(md_ctx, digest, &digest_len) == 1;
						EVP_MD_CTX_free(md_ctx);
					}
					JS_FreeCString(ctx, input);
					if (!ok) {
						return JS_ThrowInternalError(ctx, "digest computation failed");
					}
					auto hex = ToHex(digest, digest_len);
					return JS_NewString(ctx, hex.c_str());
				}

				// gdl.crypto.hmacSha256(key, data)
				JSValue HmacSha256Fn(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv) {
					if (argc < 2) {
						return JS_ThrowTypeError(ctx, "key and data are required");
					}
					size_t key_len	= 0;
					size_t data_len = 0;
					const char* key	 = JS_ToCStringLen(ctx, &key_len, argv[0]);
					const char* data = JS_ToCStringLen(ctx, &data_len, argv[1]);
					if (!key || !data) {
						if (key) {
							JS_FreeCString(ctx, key);
						}
						if (data) {
							JS_FreeCString(ctx, data);
						}
						return JS_ThrowTypeError(ctx, "key and data must be strings");
					}
					unsigned char digest[EVP_MAX_MD_SIZE];
					unsigned int digest_len = 0;
					unsigned char* result	= HMAC(EVP_sha256(), key, static_cast<int>(key_len),
												   reinterpret_cast<const unsigned char*>(data), data_len, digest,
												   &digest_len);
					JS_FreeCString(ctx, key);
					JS_FreeCString(ctx, data);
					if (!result) {
						return JS_ThrowInternalError(ctx, "hmac computation failed");
					}
					auto hex = ToHex(digest, digest_len);
					return JS_NewString(ctx, hex.c_str());
				}
			}  // namespace

			JSValue CreateCryptoApi(JSContext* ctx) {
				JSValue crypto = JS_NewObject(ctx);
				JS_SetPropertyStr(ctx, crypto, "md5",
								  JS_NewCFunctionMagic(ctx, DigestFn, "md5", 1, JS_CFUNC_generic_magic, 0));
				JS_SetPropertyStr(ctx, crypto, "sha1",
								  JS_NewCFunctionMagic(ctx, DigestFn, "sha1", 1, JS_CFUNC_generic_magic, 1));
				JS_SetPropertyStr(ctx, crypto, "sha256",
								  JS_NewCFunctionMagic(ctx, DigestFn, "sha256", 1, JS_CFUNC_generic_magic, 2));
				JS_SetPropertyStr(ctx, crypto, "hmacSha256", JS_NewCFunction(ctx, HmacSha256Fn, "hmacSha256", 2));
				return crypto;
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

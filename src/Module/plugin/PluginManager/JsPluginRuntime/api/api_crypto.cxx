#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/obj_mac.h>

#include <string>
#include <vector>

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

				// 十六进制串转字节；非法或奇数长度返回空
				std::vector<unsigned char> FromHex(const std::string& hex) {
					std::vector<unsigned char> out;
					if (hex.size() % 2 != 0) {
						return out;
					}
					auto val = [](char c) -> int {
						if (c >= '0' && c <= '9') return c - '0';
						if (c >= 'a' && c <= 'f') return c - 'a' + 10;
						if (c >= 'A' && c <= 'F') return c - 'A' + 10;
						return -1;
					};
					out.reserve(hex.size() / 2);
					for (size_t i = 0; i < hex.size(); i += 2) {
						int hi = val(hex[i]);
						int lo = val(hex[i + 1]);
						if (hi < 0 || lo < 0) {
							out.clear();
							return out;
						}
						out.push_back(static_cast<unsigned char>((hi << 4) | lo));
					}
					return out;
				}

				// gdl.crypto.secp256k1PubKey(privHex) -> 未压缩公钥 X(32)||Y(32) 的 128 位十六进制(无 04 前缀)
				JSValue Secp256k1PubKeyFn(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv) {
					if (argc < 1) {
						return JS_ThrowTypeError(ctx, "private key hex is required");
					}
					const char* ph = JS_ToCString(ctx, argv[0]);
					if (!ph) {
						return JS_ThrowTypeError(ctx, "private key must be a string");
					}
					std::string priv_hex(ph);
					JS_FreeCString(ctx, ph);
					auto priv_bytes = FromHex(priv_hex);
					if (priv_bytes.size() != 32) {
						return JS_ThrowTypeError(ctx, "private key must be 32-byte hex");
					}

					std::string result;
					bool ok			 = false;
					EC_GROUP* group	 = EC_GROUP_new_by_curve_name(NID_secp256k1);
					BN_CTX* bnctx	 = BN_CTX_new();
					BIGNUM* d		 = BN_bin2bn(priv_bytes.data(), 32, nullptr);
					EC_POINT* pub	 = group ? EC_POINT_new(group) : nullptr;
					BIGNUM* x		 = BN_new();
					BIGNUM* y		 = BN_new();
					if (group && bnctx && d && pub && x && y
						&& EC_POINT_mul(group, pub, d, nullptr, nullptr, bnctx) == 1
						&& EC_POINT_get_affine_coordinates(group, pub, x, y, bnctx) == 1) {
						unsigned char xb[32];
						unsigned char yb[32];
						if (BN_bn2binpad(x, xb, 32) == 32 && BN_bn2binpad(y, yb, 32) == 32) {
							result = ToHex(xb, 32) + ToHex(yb, 32);
							ok	   = true;
						}
					}
					if (x) BN_free(x);
					if (y) BN_free(y);
					if (pub) EC_POINT_free(pub);
					if (d) BN_free(d);
					if (bnctx) BN_CTX_free(bnctx);
					if (group) EC_GROUP_free(group);
					if (!ok) {
						return JS_ThrowInternalError(ctx, "secp256k1 pubkey derivation failed");
					}
					return JS_NewString(ctx, result.c_str());
				}

				// gdl.crypto.secp256k1Sign(privHex, hashHex) -> R(32)||S(32)||V(1) 的 130 位十六进制(low-S,含恢复位)
				JSValue Secp256k1SignFn(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv) {
					if (argc < 2) {
						return JS_ThrowTypeError(ctx, "private key hex and hash hex are required");
					}
					const char* ph = JS_ToCString(ctx, argv[0]);
					const char* hh = JS_ToCString(ctx, argv[1]);
					if (!ph || !hh) {
						if (ph) JS_FreeCString(ctx, ph);
						if (hh) JS_FreeCString(ctx, hh);
						return JS_ThrowTypeError(ctx, "private key and hash must be strings");
					}
					std::string priv_hex(ph);
					std::string hash_hex(hh);
					JS_FreeCString(ctx, ph);
					JS_FreeCString(ctx, hh);
					auto priv_bytes = FromHex(priv_hex);
					auto hash_bytes = FromHex(hash_hex);
					if (priv_bytes.size() != 32 || hash_bytes.size() != 32) {
						return JS_ThrowTypeError(ctx, "private key and hash must be 32-byte hex");
					}

					std::string result;
					bool ok			= false;
					EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
					BN_CTX* bnctx	= BN_CTX_new();
					BIGNUM* halfn	= BN_new();
					BIGNUM* d		= BN_bin2bn(priv_bytes.data(), 32, nullptr);
					BIGNUM* z		= BN_bin2bn(hash_bytes.data(), 32, nullptr);
					BIGNUM* k		= BN_new();
					BIGNUM* r		= BN_new();
					BIGNUM* s		= BN_new();
					BIGNUM* rx		= BN_new();
					BIGNUM* ry		= BN_new();
					BIGNUM* kinv	= BN_new();
					BIGNUM* rd		= BN_new();
					BIGNUM* zrd		= BN_new();
					EC_POINT* R		= group ? EC_POINT_new(group) : nullptr;
					const BIGNUM* n = group ? EC_GROUP_get0_order(group) : nullptr;
					if (group && bnctx && halfn && d && z && k && r && s && rx && ry && kinv && rd && zrd && R && n) {
						BN_rshift1(halfn, n);  // halfn = n/2,用于 low-S 规范化
						int recid = -1;
						// 随机 k 试签,直到得到合法 (r,s);同时算出恢复位 V
						for (int attempt = 0; attempt < 64 && recid < 0; ++attempt) {
							if (BN_rand_range(k, n) != 1 || BN_is_zero(k)) continue;
							if (EC_POINT_mul(group, R, k, nullptr, nullptr, bnctx) != 1) continue;
							if (EC_POINT_get_affine_coordinates(group, R, rx, ry, bnctx) != 1) continue;
							if (BN_nnmod(r, rx, n, bnctx) != 1 || BN_is_zero(r)) continue;
							int rec = BN_is_odd(ry) ? 1 : 0;
							if (BN_cmp(rx, n) >= 0) rec |= 2;
							if (!BN_mod_inverse(kinv, k, n, bnctx)) continue;
							if (BN_mod_mul(rd, r, d, n, bnctx) != 1) continue;
							if (BN_mod_add(zrd, z, rd, n, bnctx) != 1) continue;
							if (BN_mod_mul(s, kinv, zrd, n, bnctx) != 1 || BN_is_zero(s)) continue;
							if (BN_cmp(s, halfn) > 0) {  // low-S:s = n - s,恢复位奇偶翻转
								BN_sub(s, n, s);
								rec ^= 1;
							}
							recid = rec;
						}
						if (recid >= 0) {
							unsigned char rb[32];
							unsigned char sb[32];
							if (BN_bn2binpad(r, rb, 32) == 32 && BN_bn2binpad(s, sb, 32) == 32) {
								unsigned char vb = static_cast<unsigned char>(recid);
								result			 = ToHex(rb, 32) + ToHex(sb, 32) + ToHex(&vb, 1);
								ok				 = true;
							}
						}
					}
					if (halfn) BN_free(halfn);
					if (d) BN_free(d);
					if (z) BN_free(z);
					if (k) BN_free(k);
					if (r) BN_free(r);
					if (s) BN_free(s);
					if (rx) BN_free(rx);
					if (ry) BN_free(ry);
					if (kinv) BN_free(kinv);
					if (rd) BN_free(rd);
					if (zrd) BN_free(zrd);
					if (R) EC_POINT_free(R);
					if (bnctx) BN_CTX_free(bnctx);
					if (group) EC_GROUP_free(group);
					if (!ok) {
						return JS_ThrowInternalError(ctx, "secp256k1 sign failed");
					}
					return JS_NewString(ctx, result.c_str());
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
				JS_SetPropertyStr(ctx, crypto, "secp256k1PubKey",
								  JS_NewCFunction(ctx, Secp256k1PubKeyFn, "secp256k1PubKey", 1));
				JS_SetPropertyStr(ctx, crypto, "secp256k1Sign",
								  JS_NewCFunction(ctx, Secp256k1SignFn, "secp256k1Sign", 2));
				return crypto;
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

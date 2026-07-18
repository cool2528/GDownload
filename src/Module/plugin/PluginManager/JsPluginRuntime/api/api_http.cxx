#include <spdlog/spdlog.h>

#include <algorithm>
#include <cpr/cpr.h>
#include <map>
#include <sstream>
#include <vector>

#include "../net_util.h"
#include "api_context.h"

namespace gdl {
	namespace plugin {
		namespace js {

			namespace {
				constexpr int64_t kDefaultTimeoutMs = 15000;
				constexpr int64_t kMaxTimeoutMs		= 60000;
				// 响应体大小上限，防止插件拉超大文件耗尽内存
				constexpr size_t kMaxBodySize = 16ull * 1024 * 1024;

				std::string ToLowerCopy(std::string s) {
					std::transform(s.begin(), s.end(), s.begin(),
								   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
					return s;
				}

				std::string TrimCopy(const std::string& s) {
					auto begin = s.find_first_not_of(" \t\r\n");
					if (begin == std::string::npos) {
						return "";
					}
					auto end = s.find_last_not_of(" \t\r\n");
					return s.substr(begin, end - begin + 1);
				}

				// 取 JS 字符串属性；不存在返回空串
				std::string GetStringProp(JSContext* ctx, JSValueConst obj, const char* prop) {
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

				// 遍历 JS 对象的字符串键值对（值为 string 或 string 数组），回调 (key, value)
				void ForEachStringEntry(JSContext* ctx, JSValueConst obj,
										const std::function<void(const std::string&, const std::string&)>& fn) {
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
							// 数组值表达同名多值（设计文档 5.2）
							JSValue length_val = JS_GetPropertyStr(ctx, value, "length");
							int64_t length	   = 0;
							JS_ToInt64(ctx, &length, length_val);
							JS_FreeValue(ctx, length_val);
							for (int64_t j = 0; j < length; ++j) {
								JSValue item	 = JS_GetPropertyUint32(ctx, value, static_cast<uint32_t>(j));
								const char* item_str = JS_ToCString(ctx, item);
								if (item_str) {
									fn(key, item_str);
									JS_FreeCString(ctx, item_str);
								}
								JS_FreeValue(ctx, item);
							}
						} else if (!JS_IsUndefined(value) && !JS_IsNull(value)) {
							const char* value_str = JS_ToCString(ctx, value);
							if (value_str) {
								fn(key, value_str);
								JS_FreeCString(ctx, value_str);
							}
						}
						JS_FreeValue(ctx, value);
						JS_FreeCString(ctx, key);
					}
					JS_FreePropertyEnum(ctx, props, count);
				}

				// 解析 raw_header 中的所有响应头行（含重定向的中间响应），
				// 收集 Set-Cookie 并构建 headers 多值映射（键小写）
				void ParseRawHeaders(const std::string& raw_header,
									 std::multimap<std::string, std::string>& headers_out,
									 std::vector<std::string>& set_cookies_out) {
					std::istringstream iss(raw_header);
					std::string line;
					while (std::getline(iss, line)) {
						if (!line.empty() && line.back() == '\r') {
							line.pop_back();
						}
						auto colon = line.find(':');
						if (colon == std::string::npos || colon == 0) {
							continue;  // 状态行或空行
						}
						auto key   = ToLowerCopy(TrimCopy(line.substr(0, colon)));
						auto value = TrimCopy(line.substr(colon + 1));
						if (key == "set-cookie") {
							set_cookies_out.push_back(value);
						}
						headers_out.emplace(std::move(key), std::move(value));
					}
				}

				// text()/json() 的实现：func_data[0] 为响应体字符串
				JSValue ResponseTextFn(JSContext* ctx, JSValueConst /*this_val*/, int /*argc*/, JSValueConst* /*argv*/,
									   int magic, JSValueConst* func_data) {
					if (magic == 0) {
						return JS_DupValue(ctx, func_data[0]);
					}
					// json()：解析响应体
					size_t len		= 0;
					const char* str = JS_ToCStringLen(ctx, &len, func_data[0]);
					if (!str) {
						return JS_ThrowTypeError(ctx, "response body is not a string");
					}
					JSValue parsed = JS_ParseJSON(ctx, str, len, "<response>");
					JS_FreeCString(ctx, str);
					return parsed;
				}

				// 构造响应 JS 对象 { status, headers, text(), json() }
				JSValue BuildResponseObject(JSContext* ctx, long status_code,
											const std::multimap<std::string, std::string>& headers,
											const std::string& body) {
					JSValue response = JS_NewObject(ctx);
					JS_SetPropertyStr(ctx, response, "status", JS_NewInt64(ctx, status_code));

					// headers：单值为 string，多值为 string 数组
					JSValue headers_obj = JS_NewObject(ctx);
					for (auto it = headers.begin(); it != headers.end();) {
						auto range_end = headers.upper_bound(it->first);
						auto count	   = std::distance(it, range_end);
						if (count == 1) {
							JS_SetPropertyStr(ctx, headers_obj, it->first.c_str(),
											  JS_NewString(ctx, it->second.c_str()));
						} else {
							JSValue array = JS_NewArray(ctx);
							uint32_t idx  = 0;
							for (auto v = it; v != range_end; ++v) {
								JS_SetPropertyUint32(ctx, array, idx++, JS_NewString(ctx, v->second.c_str()));
							}
							JS_SetPropertyStr(ctx, headers_obj, it->first.c_str(), array);
						}
						it = range_end;
					}
					JS_SetPropertyStr(ctx, response, "headers", headers_obj);

					JSValue body_str = JS_NewStringLen(ctx, body.c_str(), body.size());
					JS_SetPropertyStr(ctx, response, "text",
									  JS_NewCFunctionData(ctx, ResponseTextFn, 0, 0, 1, &body_str));
					JS_SetPropertyStr(ctx, response, "json",
									  JS_NewCFunctionData(ctx, ResponseTextFn, 0, 1, 1, &body_str));
					JS_FreeValue(ctx, body_str);
					return response;
				}

				// gdl.http.get / gdl.http.post 主实现；magic: 0=GET 1=POST
				JSValue HttpRequest(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv,
									int magic) {
					auto* api = GetApiContext(ctx);
					if (!api || !api->manifest) {
						return JS_ThrowInternalError(ctx, "api context missing");
					}
					if (argc < 1) {
						return JS_ThrowTypeError(ctx, "url is required");
					}
					const char* url_cstr = JS_ToCString(ctx, argv[0]);
					if (!url_cstr) {
						return JS_ThrowTypeError(ctx, "url must be a string");
					}
					std::string url(url_cstr);
					JS_FreeCString(ctx, url_cstr);

					UrlParts parts;
					if (!ParseUrlParts(url, parts)) {
						return JS_ThrowTypeError(ctx, "invalid url: %s", url.c_str());
					}
					// 域名白名单强制校验（设计文档 5.1）
					if (!HostMatchesWhitelist(parts.host, api->manifest->permissions.http_domains)) {
						return JS_ThrowTypeError(ctx, "host not in permissions.http whitelist: %s",
												 parts.host.c_str());
					}

					// 解析 options
					cpr::Header header;
					cpr::Parameters parameters;
					std::string body;
					cpr::Payload form_payload{};
					std::vector<cpr::Part> multipart_parts;
					bool has_body		   = false;
					bool has_form		   = false;
					bool has_multipart	   = false;
					int64_t timeout_ms	   = kDefaultTimeoutMs;
					bool follow_redirects  = true;
					bool use_cookie_jar	   = true;
					std::string explicit_cookie;
					std::string accept_encoding;  // "" 默认自动；"identity" 禁压缩；"disabled" 不设置

					if (argc >= 2 && JS_IsObject(argv[1])) {
						JSValueConst options = argv[1];

						JSValue headers_val = JS_GetPropertyStr(ctx, options, "headers");
						if (JS_IsObject(headers_val)) {
							ForEachStringEntry(ctx, headers_val,
											   [&](const std::string& key, const std::string& value) {
												   if (ToLowerCopy(key) == "cookie") {
													   explicit_cookie = value;
												   } else {
													   header[key] = value;
												   }
											   });
						}
						JS_FreeValue(ctx, headers_val);

						JSValue params_val = JS_GetPropertyStr(ctx, options, "params");
						if (JS_IsObject(params_val)) {
							ForEachStringEntry(ctx, params_val,
											   [&](const std::string& key, const std::string& value) {
												   parameters.Add({key, value});
											   });
						}
						JS_FreeValue(ctx, params_val);

						JSValue json_val = JS_GetPropertyStr(ctx, options, "json");
						if (!JS_IsUndefined(json_val) && !JS_IsNull(json_val)) {
							JSValue json_str = JS_JSONStringify(ctx, json_val, JS_UNDEFINED, JS_UNDEFINED);
							const char* str	 = JS_ToCString(ctx, json_str);
							if (str) {
								body	 = str;
								has_body = true;
								JS_FreeCString(ctx, str);
								header["Content-Type"] = "application/json";
							}
							JS_FreeValue(ctx, json_str);
						}
						JS_FreeValue(ctx, json_val);

						// form: 对象 → application/x-www-form-urlencoded（宿主侧负责百分号编码）
						if (!has_body) {
							JSValue form_val = JS_GetPropertyStr(ctx, options, "form");
							if (JS_IsObject(form_val)) {
								ForEachStringEntry(ctx, form_val,
												   [&](const std::string& key, const std::string& value) {
													   form_payload.Add({key, value});
													   has_form = true;
												   });
							}
							JS_FreeValue(ctx, form_val);
						}

						// multipart: 对象 → multipart/form-data（清理转存文件等场景）
						if (!has_body && !has_form) {
							JSValue multipart_val = JS_GetPropertyStr(ctx, options, "multipart");
							if (JS_IsObject(multipart_val)) {
								ForEachStringEntry(ctx, multipart_val,
												   [&](const std::string& key, const std::string& value) {
													   multipart_parts.emplace_back(cpr::Part{key, value});
													   has_multipart = true;
												   });
							}
							JS_FreeValue(ctx, multipart_val);
						}

						if (!has_body && !has_form && !has_multipart) {
							auto raw_body = GetStringProp(ctx, options, "body");
							if (!raw_body.empty()) {
								body	 = std::move(raw_body);
								has_body = true;
							}
						}

						JSValue timeout_val = JS_GetPropertyStr(ctx, options, "timeout");
						if (JS_IsNumber(timeout_val)) {
							JS_ToInt64(ctx, &timeout_ms, timeout_val);
							timeout_ms = std::clamp<int64_t>(timeout_ms, 1, kMaxTimeoutMs);
						}
						JS_FreeValue(ctx, timeout_val);

						JSValue redirect_val = JS_GetPropertyStr(ctx, options, "follow_redirects");
						if (JS_IsBool(redirect_val)) {
							follow_redirects = JS_ToBool(ctx, redirect_val) != 0;
						}
						JS_FreeValue(ctx, redirect_val);

						JSValue jar_val = JS_GetPropertyStr(ctx, options, "use_cookie_jar");
						if (JS_IsBool(jar_val)) {
							use_cookie_jar = JS_ToBool(ctx, jar_val) != 0;
						}
						JS_FreeValue(ctx, jar_val);

						accept_encoding = GetStringProp(ctx, options, "accept_encoding");
					}

					// Cookie 合成：Jar 自动值 + 显式值（显式同名覆盖，设计文档 5.1.1）
					std::string cookie_header;
					if (use_cookie_jar && api->cookie_jar) {
						cookie_header = api->cookie_jar->BuildCookieHeader(parts.host, parts.path, parts.is_https);
					}
					if (!explicit_cookie.empty()) {
						if (cookie_header.empty()) {
							cookie_header = explicit_cookie;
						} else {
							// 解析显式 Cookie 的键名集合，从 Jar 值中剔除同名后拼接
							std::map<std::string, bool> explicit_names;
							std::istringstream iss(explicit_cookie);
							std::string pair;
							while (std::getline(iss, pair, ';')) {
								auto trimmed = TrimCopy(pair);
								auto eq		 = trimmed.find('=');
								if (eq != std::string::npos && eq > 0) {
									explicit_names[trimmed.substr(0, eq)] = true;
								}
							}
							std::string merged;
							std::istringstream jar_iss(cookie_header);
							while (std::getline(jar_iss, pair, ';')) {
								auto trimmed = TrimCopy(pair);
								auto eq		 = trimmed.find('=');
								if (eq == std::string::npos || explicit_names.count(trimmed.substr(0, eq))) {
									continue;
								}
								if (!merged.empty()) {
									merged += "; ";
								}
								merged += trimmed;
							}
							if (!merged.empty()) {
								merged += "; ";
							}
							merged += explicit_cookie;
							cookie_header = std::move(merged);
						}
					}
					if (!cookie_header.empty()) {
						header["Cookie"] = cookie_header;
					}

					// 执行请求（同步；异步语义由插件侧 await 保证）
					// 内容编码策略（options.accept_encoding）：
					//   ""（默认）    → 宣告 gzip/deflate 并由 libcurl 自动解压（等价 curl --compressed）；
					//                   若因中间代理破坏 gzip 流报解码错误，自动降级为 disabled 重试一次
					//   "identity"    → 仅宣告 identity（请求明文，仍开启自动解压兜底）
					//   "disabled"    → 彻底关闭：不宣告、不自动解压，原样返回响应体
					auto run_request = [&](const std::string& encoding_mode) {
						cpr::Session session;
						session.SetUrl(cpr::Url{url});
						session.SetHeader(header);
						session.SetParameters(parameters);
						session.SetTimeout(cpr::Timeout{static_cast<int32_t>(timeout_ms)});
						session.SetRedirect(cpr::Redirect{follow_redirects});
						if (encoding_mode == "disabled") {
							session.SetAcceptEncoding(cpr::AcceptEncoding{{cpr::AcceptEncodingMethods::disabled}});
						} else if (encoding_mode == "identity") {
							session.SetAcceptEncoding(cpr::AcceptEncoding{{cpr::AcceptEncodingMethods::identity}});
						} else {
							session.SetAcceptEncoding(cpr::AcceptEncoding{
								{cpr::AcceptEncodingMethods::gzip, cpr::AcceptEncodingMethods::deflate,
								 cpr::AcceptEncodingMethods::identity}});
						}
						if (has_multipart) {
							cpr::Multipart multipart{};
							multipart.parts = multipart_parts;
							session.SetMultipart(multipart);
						} else if (has_form) {
							session.SetPayload(form_payload);
						} else if (has_body) {
							session.SetBody(cpr::Body{body});
						}
						return magic == 0 ? session.Get() : session.Post();
					};

					cpr::Response response = run_request(accept_encoding);
					// 内容编码解码失败（CURLE_BAD_CONTENT_ENCODING）时，若插件未显式指定编码，
					// 降级关闭压缩重试一次，兜底中间代理破坏 gzip 流的场景
					if (response.error.code != cpr::ErrorCode::OK && accept_encoding.empty()) {
						const auto& msg			 = response.error.message;
						const bool encoding_error = msg.find("encoding") != std::string::npos
													|| msg.find("header check") != std::string::npos;
						if (encoding_error) {
							response = run_request("disabled");
						}
					}

					if (response.error.code != cpr::ErrorCode::OK) {
						return JS_ThrowPlainError(ctx, "http request failed: %s", response.error.message.c_str());
					}
					if (response.text.size() > kMaxBodySize) {
						return JS_ThrowRangeError(ctx, "response body exceeds size limit");
					}

					// 解析响应头；Set-Cookie 自动入 Jar
					std::multimap<std::string, std::string> headers;
					std::vector<std::string> set_cookies;
					ParseRawHeaders(response.raw_header, headers, set_cookies);
					if (use_cookie_jar && api->cookie_jar) {
						for (const auto& set_cookie : set_cookies) {
							api->cookie_jar->StoreFromSetCookie(set_cookie, parts.host, parts.path);
						}
					}

					return BuildResponseObject(ctx, response.status_code, headers, response.text);
				}

				// ---- gdl.http.cookies.* ----

				// Cookie 结构体转 JS 对象
				JSValue CookieToJsObject(JSContext* ctx, const Cookie& cookie) {
					JSValue obj = JS_NewObject(ctx);
					JS_SetPropertyStr(ctx, obj, "name", JS_NewString(ctx, cookie.name.c_str()));
					JS_SetPropertyStr(ctx, obj, "value", JS_NewString(ctx, cookie.value.c_str()));
					JS_SetPropertyStr(ctx, obj, "domain", JS_NewString(ctx, cookie.domain.c_str()));
					JS_SetPropertyStr(ctx, obj, "path", JS_NewString(ctx, cookie.path.c_str()));
					JS_SetPropertyStr(ctx, obj, "expires", JS_NewInt64(ctx, cookie.expires));
					JS_SetPropertyStr(ctx, obj, "secure", JS_NewBool(ctx, cookie.secure));
					JS_SetPropertyStr(ctx, obj, "http_only", JS_NewBool(ctx, cookie.http_only));
					return obj;
				}

				// magic: 0=list 1=set 2=setFromString 3=remove 4=clear
				JSValue CookiesApi(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv,
								   int magic) {
					auto* api = GetApiContext(ctx);
					if (!api || !api->cookie_jar) {
						return JS_ThrowInternalError(ctx, "cookie jar unavailable");
					}
					auto arg_string = [&](int index) -> std::string {
						if (argc <= index) {
							return "";
						}
						const char* str = JS_ToCString(ctx, argv[index]);
						if (!str) {
							return "";
						}
						std::string result(str);
						JS_FreeCString(ctx, str);
						return result;
					};

					switch (magic) {
						case 0: {  // list(domain?)
							auto cookies  = api->cookie_jar->List(arg_string(0));
							JSValue array = JS_NewArray(ctx);
							uint32_t idx  = 0;
							for (const auto& cookie : cookies) {
								JS_SetPropertyUint32(ctx, array, idx++, CookieToJsObject(ctx, cookie));
							}
							return array;
						}
						case 1: {  // set({name, value, domain, path?, expires?})
							if (argc < 1 || !JS_IsObject(argv[0])) {
								return JS_ThrowTypeError(ctx, "cookie object is required");
							}
							Cookie cookie;
							cookie.name	  = GetStringProp(ctx, argv[0], "name");
							cookie.value  = GetStringProp(ctx, argv[0], "value");
							cookie.domain = GetStringProp(ctx, argv[0], "domain");
							auto path	  = GetStringProp(ctx, argv[0], "path");
							cookie.path	  = path.empty() ? "/" : path;
							JSValue expires_val = JS_GetPropertyStr(ctx, argv[0], "expires");
							if (JS_IsNumber(expires_val)) {
								JS_ToInt64(ctx, &cookie.expires, expires_val);
							}
							JS_FreeValue(ctx, expires_val);
							cookie.host_only = true;
							if (cookie.name.empty() || cookie.domain.empty()) {
								return JS_ThrowTypeError(ctx, "cookie name/domain are required");
							}
							api->cookie_jar->Set(cookie);
							return JS_UNDEFINED;
						}
						case 2: {  // setFromString(domain, cookieString)
							auto domain		   = arg_string(0);
							auto cookie_string = arg_string(1);
							if (domain.empty() || cookie_string.empty()) {
								return JS_ThrowTypeError(ctx, "domain and cookie string are required");
							}
							api->cookie_jar->SetFromString(domain, cookie_string);
							return JS_UNDEFINED;
						}
						case 3: {  // remove(domain, name)
							auto domain = arg_string(0);
							auto name	= arg_string(1);
							if (domain.empty() || name.empty()) {
								return JS_ThrowTypeError(ctx, "domain and name are required");
							}
							api->cookie_jar->Remove(domain, name);
							return JS_UNDEFINED;
						}
						case 4:	 // clear(domain?)
							api->cookie_jar->Clear(arg_string(0));
							return JS_UNDEFINED;
						default:
							return JS_UNDEFINED;
					}
				}
			}  // namespace

			JSValue CreateHttpApi(JSContext* ctx) {
				JSValue http = JS_NewObject(ctx);
				JS_SetPropertyStr(ctx, http, "get",
								  JS_NewCFunctionMagic(ctx, HttpRequest, "get", 2, JS_CFUNC_generic_magic, 0));
				JS_SetPropertyStr(ctx, http, "post",
								  JS_NewCFunctionMagic(ctx, HttpRequest, "post", 2, JS_CFUNC_generic_magic, 1));

				JSValue cookies = JS_NewObject(ctx);
				JS_SetPropertyStr(ctx, cookies, "list",
								  JS_NewCFunctionMagic(ctx, CookiesApi, "list", 1, JS_CFUNC_generic_magic, 0));
				JS_SetPropertyStr(ctx, cookies, "set",
								  JS_NewCFunctionMagic(ctx, CookiesApi, "set", 1, JS_CFUNC_generic_magic, 1));
				JS_SetPropertyStr(ctx, cookies, "setFromString",
								  JS_NewCFunctionMagic(ctx, CookiesApi, "setFromString", 2, JS_CFUNC_generic_magic, 2));
				JS_SetPropertyStr(ctx, cookies, "remove",
								  JS_NewCFunctionMagic(ctx, CookiesApi, "remove", 2, JS_CFUNC_generic_magic, 3));
				JS_SetPropertyStr(ctx, cookies, "clear",
								  JS_NewCFunctionMagic(ctx, CookiesApi, "clear", 1, JS_CFUNC_generic_magic, 4));
				JS_SetPropertyStr(ctx, http, "cookies", cookies);
				return http;
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

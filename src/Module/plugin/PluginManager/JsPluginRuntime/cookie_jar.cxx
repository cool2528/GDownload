#include "cookie_jar.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

#include "net_util.h"

namespace gdl {
	namespace plugin {
		namespace js {

			namespace {
				int64_t NowUnixSeconds() {
					return std::chrono::duration_cast<std::chrono::seconds>(
							   std::chrono::system_clock::now().time_since_epoch())
						.count();
				}

				std::string Trim(const std::string& s) {
					auto begin = s.find_first_not_of(" \t\r\n");
					if (begin == std::string::npos) {
						return "";
					}
					auto end = s.find_last_not_of(" \t\r\n");
					return s.substr(begin, end - begin + 1);
				}

				std::string ToLowerCopy(const std::string& s) {
					std::string result(s);
					std::transform(result.begin(), result.end(), result.begin(),
								   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
					return result;
				}

				// 解析 HTTP 日期（Expires 属性），失败返回 0
				int64_t ParseHttpDate(const std::string& date_str) {
					static const char* kFormats[] = {
						"%a, %d %b %Y %H:%M:%S",  // RFC 1123: Wed, 21 Oct 2015 07:28:00 GMT
						"%d %b %Y %H:%M:%S",	  // 无星期前缀的变体
					};
					for (const auto* format : kFormats) {
						std::tm tm = {};
						std::istringstream iss(date_str);
						iss >> std::get_time(&tm, format);
						if (!iss.fail()) {
#ifdef _WIN32
							return static_cast<int64_t>(_mkgmtime(&tm));
#else
							return static_cast<int64_t>(timegm(&tm));
#endif
						}
					}
					return 0;
				}
			}  // namespace

			CookieJar::CookieJar(std::filesystem::path persist_file, std::vector<std::string> allowed_domains)
				: persist_file_(std::move(persist_file)), allowed_domains_(std::move(allowed_domains)) {}

			bool CookieJar::DomainAllowed(const std::string& domain) const {
				return HostMatchesWhitelist(domain, allowed_domains_);
			}

			void CookieJar::Load() {
				std::lock_guard<std::mutex> lock(mutex_);
				std::ifstream file(persist_file_);
				if (!file.is_open()) {
					return;
				}
				try {
					nlohmann::json json;
					file >> json;
					for (const auto& item : json) {
						Cookie cookie;
						cookie.name		 = item.value("name", "");
						cookie.value	 = item.value("value", "");
						cookie.domain	 = item.value("domain", "");
						cookie.path		 = item.value("path", "/");
						cookie.expires	 = item.value("expires", int64_t{0});
						cookie.secure	 = item.value("secure", false);
						cookie.http_only = item.value("http_only", false);
						cookie.host_only = item.value("host_only", false);
						if (!cookie.name.empty() && !cookie.domain.empty()) {
							cookies_.push_back(std::move(cookie));
						}
					}
				} catch (const nlohmann::json::exception& e) {
					spdlog::warn("[cookie-jar] failed to load {}: {}", persist_file_.generic_string(), e.what());
				}
				PurgeExpiredLocked();
			}

			void CookieJar::SaveLocked() {
				nlohmann::json json = nlohmann::json::array();
				for (const auto& cookie : cookies_) {
					// 会话 Cookie 不持久化
					if (cookie.expires == 0) {
						continue;
					}
					json.push_back({
						{"name", cookie.name},
						{"value", cookie.value},
						{"domain", cookie.domain},
						{"path", cookie.path},
						{"expires", cookie.expires},
						{"secure", cookie.secure},
						{"http_only", cookie.http_only},
						{"host_only", cookie.host_only},
					});
				}
				// 原子写入：先写临时文件再 rename
				std::error_code ec;
				std::filesystem::create_directories(persist_file_.parent_path(), ec);
				auto tmp_path = persist_file_;
				tmp_path += ".tmp";
				{
					std::ofstream file(tmp_path, std::ios::trunc);
					if (!file.is_open()) {
						spdlog::warn("[cookie-jar] cannot write {}", tmp_path.generic_string());
						return;
					}
					file << json.dump();
				}
				std::filesystem::rename(tmp_path, persist_file_, ec);
				if (ec) {
					spdlog::warn("[cookie-jar] rename failed: {}", ec.message());
				}
			}

			void CookieJar::PurgeExpiredLocked() {
				auto now = NowUnixSeconds();
				cookies_.erase(std::remove_if(cookies_.begin(), cookies_.end(),
											  [now](const Cookie& c) { return c.expires != 0 && c.expires <= now; }),
							   cookies_.end());
			}

			void CookieJar::UpsertLocked(Cookie cookie) {
				auto it = std::find_if(cookies_.begin(), cookies_.end(), [&cookie](const Cookie& c) {
					return c.name == cookie.name && c.domain == cookie.domain && c.path == cookie.path;
				});
				if (it != cookies_.end()) {
					*it = std::move(cookie);
				} else {
					cookies_.push_back(std::move(cookie));
				}
			}

			void CookieJar::StoreFromSetCookie(const std::string& set_cookie_line, const std::string& request_host,
											   const std::string& request_path) {
				// 拆分 name=value 与属性段
				std::vector<std::string> segments;
				std::istringstream iss(set_cookie_line);
				std::string segment;
				while (std::getline(iss, segment, ';')) {
					segments.push_back(Trim(segment));
				}
				if (segments.empty()) {
					return;
				}
				auto eq_pos = segments[0].find('=');
				if (eq_pos == std::string::npos || eq_pos == 0) {
					return;
				}
				Cookie cookie;
				cookie.name	 = Trim(segments[0].substr(0, eq_pos));
				cookie.value = Trim(segments[0].substr(eq_pos + 1));

				bool has_max_age = false;
				std::string domain_attr;
				for (size_t i = 1; i < segments.size(); ++i) {
					const auto& seg = segments[i];
					auto attr_eq	= seg.find('=');
					auto attr_name	= ToLowerCopy(attr_eq == std::string::npos ? seg : seg.substr(0, attr_eq));
					auto attr_value = attr_eq == std::string::npos ? "" : Trim(seg.substr(attr_eq + 1));
					if (attr_name == "domain") {
						// 去掉前导点
						domain_attr = attr_value.rfind('.', 0) == 0 ? attr_value.substr(1) : attr_value;
					} else if (attr_name == "path") {
						cookie.path = attr_value.empty() ? "/" : attr_value;
					} else if (attr_name == "max-age") {
						// Max-Age 优先于 Expires
						try {
							auto max_age   = std::stoll(attr_value);
							cookie.expires = max_age <= 0 ? 1 : NowUnixSeconds() + max_age;  // 1 = 立即过期
							has_max_age	   = true;
						} catch (...) {
						}
					} else if (attr_name == "expires" && !has_max_age) {
						cookie.expires = ParseHttpDate(attr_value);
					} else if (attr_name == "secure") {
						cookie.secure = true;
					} else if (attr_name == "httponly") {
						cookie.http_only = true;
					}
				}

				if (domain_attr.empty()) {
					// 无 Domain 属性：host-only Cookie
					cookie.domain	 = ToLowerCopy(request_host);
					cookie.host_only = true;
				} else {
					// Domain 属性必须能覆盖请求 host（防止跨域投毒）
					if (!DomainMatches(request_host, domain_attr)) {
						return;
					}
					cookie.domain = ToLowerCopy(domain_attr);
				}

				// 白名单外域名的 Cookie 静默丢弃（设计文档 5.1.1）
				if (!DomainAllowed(cookie.domain)) {
					spdlog::debug("[cookie-jar] dropped cookie for non-whitelisted domain: {}", cookie.domain);
					return;
				}
				(void)request_path;

				std::lock_guard<std::mutex> lock(mutex_);
				// 过期时间在过去 = 删除指令
				if (cookie.expires != 0 && cookie.expires <= NowUnixSeconds()) {
					cookies_.erase(std::remove_if(cookies_.begin(), cookies_.end(),
												  [&cookie](const Cookie& c) {
													  return c.name == cookie.name && c.domain == cookie.domain
															 && c.path == cookie.path;
												  }),
								   cookies_.end());
				} else {
					UpsertLocked(std::move(cookie));
				}
				SaveLocked();
			}

			std::string CookieJar::BuildCookieHeader(const std::string& host, const std::string& path, bool is_https) {
				std::lock_guard<std::mutex> lock(mutex_);
				PurgeExpiredLocked();
				std::string header;
				for (const auto& cookie : cookies_) {
					if (cookie.secure && !is_https) {
						continue;
					}
					bool domain_ok = cookie.host_only ? ToLowerCopy(host) == cookie.domain
													  : DomainMatches(host, cookie.domain);
					if (!domain_ok || !PathMatches(path, cookie.path)) {
						continue;
					}
					if (!header.empty()) {
						header += "; ";
					}
					header += cookie.name + "=" + cookie.value;
				}
				return header;
			}

			std::vector<Cookie> CookieJar::List(const std::string& domain) const {
				std::lock_guard<std::mutex> lock(mutex_);
				if (domain.empty()) {
					return cookies_;
				}
				std::vector<Cookie> result;
				for (const auto& cookie : cookies_) {
					if (DomainMatches(domain, cookie.domain) || cookie.domain == ToLowerCopy(domain)) {
						result.push_back(cookie);
					}
				}
				return result;
			}

			void CookieJar::Set(const Cookie& cookie) {
				if (!DomainAllowed(cookie.domain)) {
					spdlog::debug("[cookie-jar] Set rejected for non-whitelisted domain: {}", cookie.domain);
					return;
				}
				std::lock_guard<std::mutex> lock(mutex_);
				Cookie normalized = cookie;
				normalized.domain = ToLowerCopy(normalized.domain);
				if (normalized.path.empty()) {
					normalized.path = "/";
				}
				UpsertLocked(std::move(normalized));
				SaveLocked();
			}

			void CookieJar::SetFromString(const std::string& domain, const std::string& cookie_string) {
				if (!DomainAllowed(domain)) {
					spdlog::debug("[cookie-jar] SetFromString rejected for domain: {}", domain);
					return;
				}
				std::lock_guard<std::mutex> lock(mutex_);
				std::istringstream iss(cookie_string);
				std::string pair;
				auto lower_domain = ToLowerCopy(domain);
				while (std::getline(iss, pair, ';')) {
					auto trimmed = Trim(pair);
					auto eq_pos	 = trimmed.find('=');
					if (eq_pos == std::string::npos || eq_pos == 0) {
						continue;
					}
					Cookie cookie;
					cookie.name		 = Trim(trimmed.substr(0, eq_pos));
					cookie.value	 = Trim(trimmed.substr(eq_pos + 1));
					cookie.domain	 = lower_domain;
					cookie.host_only = true;
					// 用户注入的 Cookie 视为长期有效（一年），保证持久化
					cookie.expires = NowUnixSeconds() + 365ll * 24 * 3600;
					UpsertLocked(std::move(cookie));
				}
				SaveLocked();
			}

			void CookieJar::Remove(const std::string& domain, const std::string& name) {
				std::lock_guard<std::mutex> lock(mutex_);
				auto lower_domain = ToLowerCopy(domain);
				cookies_.erase(std::remove_if(cookies_.begin(), cookies_.end(),
											  [&](const Cookie& c) {
												  return c.domain == lower_domain && c.name == name;
											  }),
							   cookies_.end());
				SaveLocked();
			}

			void CookieJar::Clear(const std::string& domain) {
				std::lock_guard<std::mutex> lock(mutex_);
				if (domain.empty()) {
					cookies_.clear();
				} else {
					auto lower_domain = ToLowerCopy(domain);
					cookies_.erase(std::remove_if(cookies_.begin(), cookies_.end(),
												  [&](const Cookie& c) { return c.domain == lower_domain; }),
								   cookies_.end());
				}
				SaveLocked();
			}

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

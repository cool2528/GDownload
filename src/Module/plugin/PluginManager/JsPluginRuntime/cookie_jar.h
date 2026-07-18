#pragma once
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace gdl {
	namespace plugin {
		namespace js {

			// 单条 Cookie（RFC 6265 语义子集）
			struct Cookie {
				std::string name;
				std::string value;
				std::string domain;	 // 存储时不带前导点
				std::string path{"/"};
				int64_t expires{0};	 // Unix 秒；0 表示会话 Cookie（不持久化）
				bool secure{false};
				bool http_only{false};
				bool host_only{false};	// Set-Cookie 未带 Domain 属性时为 true，仅精确匹配 host
			};

			// 每插件独立的 Cookie Jar（设计文档 5.1.1）
			// 线程安全；持久 Cookie 自动落盘（原子写入），会话 Cookie 仅存内存
			class CookieJar {
			   public:
				// persist_file: plugin_cookies/<name>.json
				// allowed_domains: manifest permissions.http 白名单，域外 Cookie 静默丢弃
				CookieJar(std::filesystem::path persist_file, std::vector<std::string> allowed_domains);

				// 从磁盘加载持久 Cookie（过期条目自动清理）
				void Load();

				// 解析一条 Set-Cookie 响应头并入库；域不在白名单或格式非法时丢弃
				void StoreFromSetCookie(const std::string& set_cookie_line, const std::string& request_host,
										const std::string& request_path);

				// 构造请求应携带的 Cookie 头值（"k1=v1; k2=v2"）；无匹配返回空串
				std::string BuildCookieHeader(const std::string& host, const std::string& path, bool is_https);

				// ---- 管理 API（对应 gdl.http.cookies.*）----
				std::vector<Cookie> List(const std::string& domain) const;
				void Set(const Cookie& cookie);
				// 解析 "k1=v1; k2=v2" 批量注入（用户粘贴 Cookie 场景），domain 为 host-only 域
				void SetFromString(const std::string& domain, const std::string& cookie_string);
				void Remove(const std::string& domain, const std::string& name);
				// domain 为空则清空整个 Jar
				void Clear(const std::string& domain);

			   private:
				// 持久化（调用方需持锁）
				void SaveLocked();
				// 删除过期条目（调用方需持锁）
				void PurgeExpiredLocked();
				bool DomainAllowed(const std::string& domain) const;
				// 新增或替换同 (name, domain, path) 条目（调用方需持锁）
				void UpsertLocked(Cookie cookie);

			   private:
				std::filesystem::path persist_file_;
				std::vector<std::string> allowed_domains_;
				std::vector<Cookie> cookies_;
				mutable std::mutex mutex_;
			};

		}  // namespace js
	}  // namespace plugin
}  // namespace gdl

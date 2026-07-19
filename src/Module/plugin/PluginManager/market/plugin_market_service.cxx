#include "plugin_market_service.h"

#include <miniz/miniz.h>
#include <openssl/evp.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cpr/cpr.h>
#include <cstring>
#include <ctime>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>

#include "../JsPluginRuntime/plugin_manifest.h"

namespace gdl {
	namespace market {

		namespace {
			// 解析 https://raw.githubusercontent.com/<owner>/<repo>/<branch>/<path> 的四段
			bool ParseRawGithub(const std::string& url, std::string& owner, std::string& repo,
								std::string& branch, std::string& path) {
				const std::string host = "raw.githubusercontent.com/";
				auto pos			   = url.find(host);
				if (pos == std::string::npos) {
					return false;
				}
				std::string rest = url.substr(pos + host.size());
				auto q			 = rest.find('?');
				if (q != std::string::npos) {
					rest = rest.substr(0, q);
				}
				// 前三段为 owner/repo/branch，其余为 path
				std::string* fields[3] = {&owner, &repo, &branch};
				size_t start		   = 0;
				int idx				   = 0;
				for (; idx < 3; ++idx) {
					auto slash = rest.find('/', start);
					if (slash == std::string::npos) {
						return false;
					}
					*fields[idx] = rest.substr(start, slash - start);
					start		 = slash + 1;
				}
				path = rest.substr(start);
				return !owner.empty() && !repo.empty() && !branch.empty() && !path.empty();
			}

			// 注册表 Ed25519 公钥（raw 32 字节的 base64），对应 gdownload-plugin-registry/keys
			constexpr const char* kRegistryPublicKeyB64 = "NTXbQg1oeXn+HePHCGRi4XHagyLhBKkMOe3ODeBvOPs=";
			constexpr int kDownloadTimeoutMs			 = 20000;
			constexpr std::uint64_t kMaxPackageBytes	 = 32ull * 1024 * 1024;

			std::string Base64Decode(const std::string& in) {
				std::string out;
				out.resize((in.size() / 4 + 1) * 3);
				int len = EVP_DecodeBlock(reinterpret_cast<unsigned char*>(out.data()),
										  reinterpret_cast<const unsigned char*>(in.data()),
										  static_cast<int>(in.size()));
				if (len < 0) {
					return {};
				}
				// 按填充修正长度
				size_t padding = 0;
				if (!in.empty() && in.back() == '=') {
					++padding;
				}
				if (in.size() >= 2 && in[in.size() - 2] == '=') {
					++padding;
				}
				out.resize(static_cast<size_t>(len) - padding);
				return out;
			}
		}  // namespace

		std::string Sha256Hex(const std::string& bytes) {
			unsigned char digest[EVP_MAX_MD_SIZE];
			unsigned int len   = 0;
			EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
			std::string hex;
			if (md_ctx
				&& EVP_DigestInit_ex(md_ctx, EVP_sha256(), nullptr) == 1
				&& EVP_DigestUpdate(md_ctx, bytes.data(), bytes.size()) == 1
				&& EVP_DigestFinal_ex(md_ctx, digest, &len) == 1) {
				static const char* kHex = "0123456789abcdef";
				hex.reserve(len * 2);
				for (unsigned int i = 0; i < len; ++i) {
					hex.push_back(kHex[digest[i] >> 4]);
					hex.push_back(kHex[digest[i] & 0x0f]);
				}
			}
			if (md_ctx) {
				EVP_MD_CTX_free(md_ctx);
			}
			return hex;
		}

		bool VerifyEd25519(const std::string& bytes, const std::string& signature_base64) {
			auto raw_key = Base64Decode(kRegistryPublicKeyB64);
			auto sig	 = Base64Decode(signature_base64);
			if (raw_key.size() != 32 || sig.empty()) {
				return false;
			}
			EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(
				EVP_PKEY_ED25519, nullptr, reinterpret_cast<const unsigned char*>(raw_key.data()), raw_key.size());
			if (!pkey) {
				return false;
			}
			bool ok			   = false;
			EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
			if (md_ctx && EVP_DigestVerifyInit(md_ctx, nullptr, nullptr, nullptr, pkey) == 1) {
				int r = EVP_DigestVerify(md_ctx, reinterpret_cast<const unsigned char*>(sig.data()), sig.size(),
										 reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
				ok	  = (r == 1);
			}
			if (md_ctx) {
				EVP_MD_CTX_free(md_ctx);
			}
			EVP_PKEY_free(pkey);
			return ok;
		}

		const PluginVersion* RegistryPlugin::FindVersion(const std::string& version) const {
			for (const auto& v : versions) {
				if (v.version == version) {
					return &v;
				}
			}
			return nullptr;
		}

		const PluginVersion* RegistryPlugin::LatestVersion() const { return FindVersion(latest); }

		PluginMarketService::PluginMarketService(std::filesystem::path plugins_dir, std::filesystem::path data_dir)
			: plugins_dir_(std::move(plugins_dir)), data_dir_(std::move(data_dir)) {}

		bool PluginMarketService::FetchRegistry(const std::vector<std::string>& registry_urls,
											   std::string& error_out) {
			std::string last_error = "no registry url";
			// 破 CDN 缓存：raw.githubusercontent / jsDelivr 会缓存 registry.json，
			// 追加变化的查询参数并带 no-cache 头，确保拿到最新的可更新信息
			auto cache_buster = std::to_string(static_cast<long long>(std::time(nullptr)));
			for (const auto& url : registry_urls) {
				std::string fetch_url = url + (url.find('?') == std::string::npos ? "?" : "&") + "_cb=" + cache_buster;
				cpr::Session session;
				session.SetUrl(cpr::Url{fetch_url});
				session.SetHeader(cpr::Header{{"Cache-Control", "no-cache"}, {"Pragma", "no-cache"}});
				session.SetTimeout(cpr::Timeout{kDownloadTimeoutMs});
				session.SetRedirect(cpr::Redirect{true});
				auto resp = session.Get();
				if (resp.error.code != cpr::ErrorCode::OK || resp.status_code != 200) {
					last_error = "fetch failed (" + std::to_string(resp.status_code) + "): " + url;
					spdlog::warn("[market] registry source failed, trying next: {}", last_error);
					continue;
				}
				try {
					auto json = nlohmann::json::parse(resp.text);
					if (json.value("registry_version", 0) != 1) {
						last_error = "unsupported registry_version";
						continue;
					}
					std::vector<RegistryPlugin> parsed;
					for (const auto& p : json.at("plugins")) {
						RegistryPlugin plugin;
						plugin.name			= p.at("name").get<std::string>();
						plugin.display_name = p.value("display_name", plugin.name);
						plugin.description	= p.value("description", "");
						plugin.author		= p.value("author", "");
						plugin.homepage		= p.value("homepage", "");
						plugin.type			= p.value("type", "netdisk");
						plugin.url_patterns = p.value("url_patterns", std::vector<std::string>{});
						plugin.verified		= p.value("verified", false);
						plugin.latest		= p.at("latest").get<std::string>();
						if (p.contains("locales") && p["locales"].is_object()) {
							for (auto it = p["locales"].begin(); it != p["locales"].end(); ++it) {
								if (!it.value().is_object()) {
									continue;
								}
								plugin::js::LocaleStrings ls;
								ls.display_name		 = it.value().value("display_name", "");
								ls.description		 = it.value().value("description", "");
								plugin.locales[it.key()] = std::move(ls);
							}
						}
						for (const auto& v : p.at("versions")) {
							PluginVersion ver;
							ver.version			= v.at("version").get<std::string>();
							ver.min_app_version = v.value("min_app_version", "0.0.0");
							ver.size			= v.value("size", std::uint64_t{0});
							ver.sha256			= v.at("sha256").get<std::string>();
							ver.signature		= v.at("signature").get<std::string>();
							ver.download_urls	= v.at("download_urls").get<std::vector<std::string>>();
							plugin.versions.push_back(std::move(ver));
						}
						parsed.push_back(std::move(plugin));
					}
					registry_ = std::move(parsed);
					spdlog::info("[market] registry loaded: {} plugins from {}", registry_.size(), url);
					return true;
				} catch (const nlohmann::json::exception& e) {
					last_error = std::string("registry parse error: ") + e.what();
					continue;
				}
			}
			error_out = last_error;
			return false;
		}

		std::vector<plugin::js::PluginManifest> PluginMarketService::ScanInstalled() const {
			std::vector<plugin::js::PluginManifest> result;
			std::error_code ec;
			if (!std::filesystem::exists(plugins_dir_, ec)) {
				return result;
			}
			for (const auto& entry : std::filesystem::directory_iterator(plugins_dir_, ec)) {
				if (!entry.is_directory()) {
					continue;
				}
				std::string err;
				auto manifest = plugin::js::LoadManifest(entry.path(), err);
				if (manifest) {
					result.push_back(std::move(*manifest));
				}
			}
			return result;
		}

		std::vector<std::string> PluginMarketService::DisabledPlugins() const {
			std::vector<std::string> disabled;
			auto state_file = data_dir_ / "plugin_state.json";
			std::ifstream f(state_file);
			if (!f.is_open()) {
				return disabled;
			}
			try {
				nlohmann::json json;
				f >> json;
				if (json.contains("disabled") && json["disabled"].is_array()) {
					disabled = json["disabled"].get<std::vector<std::string>>();
				}
			} catch (const nlohmann::json::exception&) {
			}
			return disabled;
		}

		std::vector<MarketItem> PluginMarketService::ComputeItems() const {
			auto installed = ScanInstalled();
			auto disabled  = DisabledPlugins();
			auto is_disabled = [&](const std::string& name) {
				return std::find(disabled.begin(), disabled.end(), name) != disabled.end();
			};
			auto installed_ver = [&](const std::string& name) -> std::string {
				for (const auto& m : installed) {
					if (m.name == name) {
						return m.version;
					}
				}
				return {};
			};

			// 按当前 locale 解析本地化文案，缺失回退默认
			auto localize = [this](const std::map<std::string, plugin::js::LocaleStrings>& locales,
								   const std::string& def_name, const std::string& def_desc,
								   std::string& out_name, std::string& out_desc) {
				out_name = def_name;
				out_desc = def_desc;
				if (locale_.empty()) {
					return;
				}
				auto it = locales.find(locale_);
				if (it != locales.end()) {
					if (!it->second.display_name.empty()) {
						out_name = it->second.display_name;
					}
					if (!it->second.description.empty()) {
						out_desc = it->second.description;
					}
				}
			};

			std::vector<MarketItem> items;
			std::vector<std::string> shown;  // 已展示的插件名，避免重复
			// 1) 注册表插件（含在架与可安装项）
			for (const auto& plugin : registry_) {
				MarketItem item;
				item.meta			   = plugin;
				localize(plugin.locales, plugin.display_name, plugin.description, item.meta.display_name,
						 item.meta.description);
				item.installed_version = installed_ver(plugin.name);
				item.enabled		   = !is_disabled(plugin.name);
				if (item.installed_version.empty()) {
					item.state = InstallState::Available;
				} else if (plugin::js::CompareSemver(item.installed_version, plugin.latest) < 0) {
					item.state = InstallState::UpdateAvailable;
				} else {
					item.state = InstallState::Installed;
				}
				shown.push_back(plugin.name);
				items.push_back(std::move(item));
			}
			// 2) 本地已装但不在注册表的插件（离线优先：注册表未加载时也能立即展示）
			for (const auto& m : installed) {
				if (std::find(shown.begin(), shown.end(), m.name) != shown.end()) {
					continue;
				}
				MarketItem item;
				item.meta.name			= m.name;
				localize(m.locales, m.display_name, m.description, item.meta.display_name, item.meta.description);
				item.meta.author		= m.author;
				item.meta.homepage		= m.homepage;
				item.meta.type			= m.type;
				item.meta.url_patterns	= m.url_patterns;
				item.meta.verified		= false;  // 本地插件无签名背书信息
				item.meta.latest		= m.version;
				item.installed_version	= m.version;
				item.enabled			= !is_disabled(m.name);
				item.state				= InstallState::Installed;
				items.push_back(std::move(item));
			}
			return items;
		}

		std::vector<std::string> ExpandMirrorUrls(const std::vector<std::string>& base,
												  const std::string& user_proxy) {
			// ghproxy 风格前缀镜像（代理整条 github.com / raw.githubusercontent.com URL）
			static const char* kGhProxies[] = {
				"https://ghfast.top/", "https://gh-proxy.com/", "https://mirror.ghproxy.com/"};
			// jsDelivr 各节点（中国可达性各异，多试几个）
			static const char* kJsdHosts[] = {"cdn.jsdelivr.net", "fastly.jsdelivr.net", "gcore.jsdelivr.net",
											  "cdn.jsdmirror.com"};

			std::vector<std::string> out;
			std::set<std::string> seen;
			auto add = [&](const std::string& u) {
				if (!u.empty() && seen.insert(u).second) {
					out.push_back(u);
				}
			};
			// 仅「裸 github/raw 开头」的 URL 适合加代理前缀（避免给已代理的 URL 再套一层）
			auto is_bare_github = [](const std::string& u) {
				return u.rfind("https://github.com/", 0) == 0
					|| u.rfind("https://raw.githubusercontent.com/", 0) == 0;
			};

			for (const auto& url : base) {
				// 1. 用户自定义代理前缀（仅对裸 GitHub URL，放最前优先尝试）
				if (!user_proxy.empty() && is_bare_github(url)) {
					std::string p = user_proxy;
					if (p.back() != '/') {
						p += '/';
					}
					add(p + url);
				}
				// 2. 原始 URL
				add(url);
				// 3. 内置 ghproxy 前缀镜像
				if (is_bare_github(url)) {
					for (auto* pfx : kGhProxies) {
						add(std::string(pfx) + url);
					}
				}
				// 4. raw.githubusercontent → jsDelivr 各节点
				std::string o, r, b, path;
				if (ParseRawGithub(url, o, r, b, path)) {
					for (auto* h : kJsdHosts) {
						add("https://" + std::string(h) + "/gh/" + o + "/" + r + "@" + b + "/" + path);
					}
				}
				// 5. cdn.jsdelivr.net/gh/... → 其它 jsDelivr 节点
				auto gh = url.find("jsdelivr.net/gh/");
				if (gh != std::string::npos) {
					std::string tail = url.substr(url.find("/gh/"));  // "/gh/owner/repo@branch/path"
					for (auto* h : kJsdHosts) {
						add("https://" + std::string(h) + tail);
					}
				}
			}
			return out;
		}

		std::optional<std::string> PluginMarketService::DownloadWithFallback(
			const std::vector<std::string>& urls, const ProgressCallback& progress, std::string& error_out) const {
			std::string last_error = "no download url";
			// 扩展为更多中国可达镜像 + 用户自定义代理
			const std::vector<std::string> expanded = ExpandMirrorUrls(urls, user_proxy_);
			for (size_t i = 0; i < expanded.size(); ++i) {
				const auto& url = expanded[i];
				if (progress) {
					progress(5, "downloading (source " + std::to_string(i + 1) + "/"
									 + std::to_string(expanded.size()) + ")");
				}
				cpr::Session session;
				session.SetUrl(cpr::Url{url});
				session.SetTimeout(cpr::Timeout{kDownloadTimeoutMs});
				session.SetRedirect(cpr::Redirect{true});
				auto resp = session.Get();
				if (resp.error.code != cpr::ErrorCode::OK || resp.status_code != 200) {
					last_error = "download failed (" + std::to_string(resp.status_code) + "): " + url;
					spdlog::warn("[market] download source failed, trying next: {}", last_error);
					continue;
				}
				if (resp.text.size() > kMaxPackageBytes) {
					last_error = "package exceeds size limit";
					continue;
				}
				return resp.text;
			}
			error_out = last_error;
			return std::nullopt;
		}

		bool PluginMarketService::ExtractZip(const std::string& zip_bytes, const std::string& name,
											std::string& error_out) const {
			mz_zip_archive zip;
			std::memset(&zip, 0, sizeof(zip));
			if (!mz_zip_reader_init_mem(&zip, zip_bytes.data(), zip_bytes.size(), 0)) {
				error_out = "invalid zip archive";
				return false;
			}

			// 原子替换：先解压到临时目录，成功后替换正式目录
			auto target_dir = plugins_dir_ / name;
			auto tmp_dir	= plugins_dir_ / (name + ".installing");
			std::error_code ec;
			std::filesystem::remove_all(tmp_dir, ec);
			std::filesystem::create_directories(tmp_dir, ec);

			bool ok		 = true;
			mz_uint count = mz_zip_reader_get_num_files(&zip);
			for (mz_uint i = 0; i < count; ++i) {
				mz_zip_archive_file_stat st;
				if (!mz_zip_reader_file_stat(&zip, i, &st)) {
					ok		  = false;
					error_out = "zip stat failed";
					break;
				}
				std::string entry_name = st.m_filename;
				// 防目录穿越：拒绝绝对路径与 ..
				if (entry_name.empty() || entry_name.front() == '/' || entry_name.find("..") != std::string::npos
					|| entry_name.find(':') != std::string::npos) {
					ok		  = false;
					error_out = "unsafe zip entry: " + entry_name;
					break;
				}
				auto out_path = tmp_dir / entry_name;
				if (mz_zip_reader_is_file_a_directory(&zip, i)) {
					std::filesystem::create_directories(out_path, ec);
					continue;
				}
				std::filesystem::create_directories(out_path.parent_path(), ec);
				size_t out_size = 0;
				void* p			= mz_zip_reader_extract_to_heap(&zip, i, &out_size, 0);
				if (!p) {
					ok		  = false;
					error_out = "zip extract failed: " + entry_name;
					break;
				}
				std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
				if (out.is_open()) {
					out.write(static_cast<const char*>(p), static_cast<std::streamsize>(out_size));
				} else {
					ok		  = false;
					error_out = "write failed: " + entry_name;
				}
				mz_free(p);
				if (!ok) {
					break;
				}
			}
			mz_zip_reader_end(&zip);

			// 校验解出的包含 manifest.json 且 name 一致
			if (ok) {
				std::string err;
				auto manifest = plugin::js::LoadManifest(tmp_dir, err);
				if (!manifest) {
					ok		  = false;
					error_out = "package manifest invalid: " + err;
				} else if (manifest->name != name) {
					ok		  = false;
					error_out = "package name mismatch: " + manifest->name + " != " + name;
				}
			}

			if (!ok) {
				std::filesystem::remove_all(tmp_dir, ec);
				return false;
			}
			// 替换正式目录
			std::filesystem::remove_all(target_dir, ec);
			std::filesystem::rename(tmp_dir, target_dir, ec);
			if (ec) {
				error_out = "install replace failed: " + ec.message();
				std::filesystem::remove_all(tmp_dir, ec);
				return false;
			}
			return true;
		}

		bool PluginMarketService::InstallPlugin(const std::string& name, const std::string& version,
											   const ProgressCallback& progress, std::string& error_out) {
			// 定位 registry 版本条目
			const RegistryPlugin* plugin = nullptr;
			for (const auto& p : registry_) {
				if (p.name == name) {
					plugin = &p;
					break;
				}
			}
			if (!plugin) {
				error_out = "plugin not in registry: " + name;
				return false;
			}
			const PluginVersion* ver = plugin->FindVersion(version);
			if (!ver) {
				error_out = "version not found: " + version;
				return false;
			}

			auto zip_bytes = DownloadWithFallback(ver->download_urls, progress, error_out);
			if (!zip_bytes) {
				return false;
			}

			if (progress) {
				progress(70, "verifying");
			}
			// SHA-256 校验
			auto digest = Sha256Hex(*zip_bytes);
			if (digest != ver->sha256) {
				error_out = "sha256 mismatch (expected " + ver->sha256 + ", got " + digest + ")";
				return false;
			}
			// Ed25519 签名校验
			if (!VerifyEd25519(*zip_bytes, ver->signature)) {
				error_out = "signature verification failed";
				return false;
			}

			if (progress) {
				progress(85, "extracting");
			}
			if (!ExtractZip(*zip_bytes, name, error_out)) {
				return false;
			}
			if (progress) {
				progress(100, "done");
			}
			spdlog::info("[market] installed {} v{}", name, version);
			return true;
		}

		bool PluginMarketService::UninstallPlugin(const std::string& name, std::string& error_out) {
			auto target = plugins_dir_ / name;
			std::error_code ec;
			if (!std::filesystem::exists(target, ec)) {
				error_out = "not installed: " + name;
				return false;
			}
			std::filesystem::remove_all(target, ec);
			if (ec) {
				error_out = "uninstall failed: " + ec.message();
				return false;
			}
			spdlog::info("[market] uninstalled {}", name);
			return true;
		}

		bool PluginMarketService::SetEnabled(const std::string& name, bool enabled, std::string& error_out) {
			auto disabled = DisabledPlugins();
			auto it		  = std::find(disabled.begin(), disabled.end(), name);
			if (enabled && it != disabled.end()) {
				disabled.erase(it);
			} else if (!enabled && it == disabled.end()) {
				disabled.push_back(name);
			} else {
				return true;  // 无变化
			}
			nlohmann::json json;
			json["disabled"] = disabled;
			std::error_code ec;
			std::filesystem::create_directories(data_dir_, ec);
			auto state_file = data_dir_ / "plugin_state.json";
			auto tmp_file	= state_file;
			tmp_file += ".tmp";
			{
				std::ofstream out(tmp_file, std::ios::trunc);
				if (!out.is_open()) {
					error_out = "cannot write plugin_state.json";
					return false;
				}
				out << json.dump(2);
			}
			std::filesystem::rename(tmp_file, state_file, ec);
			if (ec) {
				error_out = "state write failed: " + ec.message();
				return false;
			}
			return true;
		}

	}  // namespace market
}  // namespace gdl

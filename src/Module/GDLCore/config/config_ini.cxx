#include "config_ini.h"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "config_key.h"
#include "detail/toml_config_save.h"
#include "logger.h"
#include "os/os.h"
#include "spdlog/spdlog.h"
namespace pt = boost::property_tree;
namespace gdl {
	namespace config {

		namespace {
			inline std::string ToLower(std::string s) {
				std::transform(s.begin(), s.end(), s.begin(),
							   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				return s;
			}

			inline bool IsIntegerString(const std::string& s) {
				if (s.empty()) return false;
				size_t i = 0;
				if (s[0] == '+' || s[0] == '-') {
					if (s.size() == 1) return false;
					i = 1;
				}
				for (; i < s.size(); ++i) {
					if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
				}
				return true;
			}

			inline bool ParseBoolLike(const std::string& s, bool& out) {
				auto ls = ToLower(s);
				if (ls == "true" || ls == "1" || ls == "yes" || ls == "on") {
					out = true;
					return true;
				}
				if (ls == "false" || ls == "0" || ls == "no" || ls == "off") {
					out = false;
					return true;
				}
				return false;
			}

			inline std::string BuildTrackerJson(const std::map<std::string, std::vector<std::string>>& source,
												bool use_name) {
				nlohmann::json json_data = nlohmann::json::array();
				for (const auto& item : source) {
					if (use_name) {
						json_data.push_back(item.first);
					} else {
						// 展开该逻辑源的全部镜像 URL
						for (const auto& url : item.second) json_data.push_back(url);
					}
				}
				return json_data.dump();
			}

			// TrackerSourceNames 的默认值：只选两家的精选列表（与引擎侧 DefaultTrackerSourceNames 保持一致），
			// 全量/分协议列表由用户按需勾选
			inline std::string DefaultTrackerSourceNamesJson() {
				return nlohmann::json::array({"ngosang-best", "XIU2-best"}).dump();
			}

			inline std::string EnsureSessionPath() {
				std::string path = os::GetAppDataDir() + "/gdownload/session/aria2.session";
				std::error_code ec;
				auto session_dir = std::filesystem::path(path).parent_path();
				if (!std::filesystem::exists(session_dir, ec)) {
					std::filesystem::create_directories(session_dir, ec);
				}
				return path;
			}

			inline std::string GenerateRpcSecret() {
				static constexpr char kChars[] =
					"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
				std::random_device rd;
				std::uniform_int_distribution<std::size_t> dist(0, sizeof(kChars) - 2);

				std::string secret;
				secret.reserve(32);
				for (int i = 0; i < 32; ++i) {
					secret.push_back(kChars[dist(rd)]);
				}
				return secret;
			}
		}  // namespace

		ApplicationConfig::~ApplicationConfig() {
			// 程序退出阶段 spdlog 可能已析构，且其他线程可能仍持有锁。
			// 用 try_lock 避免死锁，通过无日志 helper 尽力原子保存。
			try {
				std::unique_lock lock(mutex_, std::defer_lock);
				if (lock.try_lock()) {
					(void) detail::SaveTomlAtomically(config_file_path_, toml_root_);
				}
			} catch (...) {
				// 析构中吞掉所有异常
			}
		}

		std::vector<std::string> ApplicationConfig::GetTrackerServerUrlsByName(const std::string& name) const {
			auto it = tracker_source_server_.find(NormalizeTrackerSourceName(name));
			if (it != tracker_source_server_.end()) {
				return it->second;
			}
			return {};
		}

		std::string ApplicationConfig::NormalizeTrackerSourceName(const std::string& name) const {
			// 已是逻辑源名则直接返回
			if (tracker_source_server_.find(name) != tracker_source_server_.end()) {
				return name;
			}
			// 旧版按「源×通道」拆分的名称形如 "ngosang-best-link/-mirror/-cdn"，
			// 去掉通道后缀后若命中逻辑源名则归一化
			static const std::vector<std::string> legacy_suffixes = {"-link", "-mirror", "-cdn"};
			for (const auto& suffix : legacy_suffixes) {
				if (name.size() > suffix.size() &&
					name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0) {
					auto stripped = name.substr(0, name.size() - suffix.size());
					if (tracker_source_server_.find(stripped) != tracker_source_server_.end()) {
						return stripped;
					}
				}
			}
			return name;
		}

		ApplicationConfig::ApplicationConfig() {
			config_file_path_ = os::GetAppDataDir() + "/gdownload/gd.toml";
			legacy_config_file_path_ = os::GetAppDataDir() + "/gdownload/gd.ini";
			// 逻辑源 -> 有序镜像列表：主源在前，回退镜像在后。
			// 镜像通道（raw/github.io/jsDelivr 等）由引擎按序自动回退，不再作为独立源暴露给用户。
			// ngosang/trackerslist：每日更新的公共 tracker 精选列表
			auto ngosang_mirrors = [](const char* file) {
				return std::vector<std::string>{
					std::string("https://raw.githubusercontent.com/ngosang/trackerslist/master/") + file,
					std::string("https://ngosang.github.io/trackerslist/") + file,
					std::string("https://cdn.jsdelivr.net/gh/ngosang/trackerslist@master/") + file,
				};
			};
			tracker_source_server_["ngosang-best"]		= ngosang_mirrors("trackers_best.txt");
			tracker_source_server_["ngosang-best-ip"]	= ngosang_mirrors("trackers_best_ip.txt");
			tracker_source_server_["ngosang-all"]		= ngosang_mirrors("trackers_all.txt");
			tracker_source_server_["ngosang-all_udp"]	= ngosang_mirrors("trackers_all_udp.txt");
			tracker_source_server_["ngosang-all_http"]	= ngosang_mirrors("trackers_all_http.txt");
			tracker_source_server_["ngosang-all_https"] = ngosang_mirrors("trackers_all_https.txt");
			// XIU2/TrackersListCollection：每日更新，自带国内可达镜像
			auto xiu2_mirrors = [](const char* file) {
				return std::vector<std::string>{
					std::string("https://cf.trackerslist.com/") + file,
					std::string("https://raw.githubusercontent.com/XIU2/TrackersListCollection/master/") + file,
					std::string("https://jsd.onmicrosoft.cn/gh/XIU2/TrackersListCollection/") + file,
					std::string("https://cdn.jsdelivr.net/gh/XIU2/TrackersListCollection@master/") + file,
				};
			};
			tracker_source_server_["XIU2-best"]	  = xiu2_mirrors("best.txt");
			tracker_source_server_["XIU2-all"]	  = xiu2_mirrors("all.txt");
			tracker_source_server_["XIU2-http"]	  = xiu2_mirrors("http.txt");
			tracker_source_server_["XIU2-nohttp"] = xiu2_mirrors("nohttp.txt");
			// newTrackon：持续监测公共 tracker 可用率，stable 为可用率 >= 95% 的列表
			tracker_source_server_["newtrackon-stable"] = {"https://newtrackon.com/api/stable"};

			EnsureConfigFileExists();
			Load();
		}

		bool ApplicationConfig::Load() {
			try {
				std::unique_lock lock(mutex_);
				EnsureConfigFileExists();
				toml_root_.clear();

				bool parsed_toml = false;
				try {
					std::error_code ec;
					if (std::filesystem::file_size(config_file_path_, ec) > 0 && !ec) {
#if TOML_EXCEPTIONS
						toml_root_ = toml::parse_file(config_file_path_);
						parsed_toml = true;
#else
						auto parse_result = toml::parse_file(config_file_path_);
						if (parse_result) {
							toml_root_ = std::move(parse_result).table();
							parsed_toml = true;
						} else {
							const auto& err = parse_result.error();
							LOG_ERR("Failed to parse {}: {} (line {}, column {})", config_file_path_, err.description(),
									err.source().begin.line, err.source().begin.column);
						}
#endif
					}
				} catch (const toml::parse_error& err) {
					LOG_ERR("Failed to parse {}: {} (line {}, column {})", config_file_path_, err.description(),
							err.source().begin.line, err.source().begin.column);
				} catch (const std::exception& e) {
					LOG_ERR("Unexpected error while parsing {}: {}", config_file_path_, e.what());
				}

				if (!parsed_toml || toml_root_.empty()) {
					TryMigrateFromLegacyIniUnlocked();
				}

				auto all_paths = config::Keys::GetAllKeys();
				auto all_values = config::Keys::GetAllValues();
				for (std::size_t i = 0; i < all_paths.size(); ++i) {
					auto key_path = std::string(all_paths[i].data());
					std::string def = std::string(all_values[i].data());
					LOG_DBG("KEY {} VALUE {}", key_path, def);

					auto cur_opt = TryGetStringUnlocked(key_path);
					if (!cur_opt.has_value()) {
						auto value = ResolveDefaultValue(key_path, def);
						SetValueInternalUnlocked(key_path, value);
					} else {
						auto maybe_update = ValidateExistingValue(key_path, *cur_opt, def);
						if (maybe_update.has_value()) {
							SetValueInternalUnlocked(key_path, *maybe_update);
						}
					}
				}
				lock.unlock();
				Save();
			} catch (std::exception& e) {
				LOG_ERR("Load config fail error {}", e.what());
				return false;
			}
			return true;
		}

		bool ApplicationConfig::Save() {
			try {
				std::unique_lock lock(mutex_);
				const auto result = detail::SaveTomlAtomically(config_file_path_, toml_root_);
				if (!result) {
					LOG_ERR("Failed to save TOML configuration at atomic replacement stage {}: {}",
							result.GetError().Code(), result.GetError().what());
					return false;
				}
			} catch (std::exception& e) {
				LOG_ERR("save toml fail error {}", e.what());
				return false;
			}
			return true;
		}

		bool ApplicationConfig::EnsureConfigFileExists() {
			std::error_code ec;
			auto config_dir = std::filesystem::path(config_file_path_).parent_path();
			std::filesystem::create_directories(config_dir, ec);
			if (ec) {
				LOG_ERR("Failed to create config directory: {}", ec.message());
				return false;
			}

			if (!std::filesystem::exists(config_file_path_, ec)) {
				try {
					std::ofstream config(config_file_path_);
					if (!config.is_open()) {
						LOG_ERR("Failed to create config file: {}", config_file_path_);
						return false;
					}
					config.close();
				} catch (const std::exception& e) {
					LOG_ERR("Failed to create config file: {}", e.what());
					return false;
				}
			}
			return true;
		}

		void ApplicationConfig::SetValueInternalUnlocked(const std::string& key, const std::string& value) {
			auto segments = SplitKeyPath(key);
			if (segments.empty()) return;

			toml::table* table = &toml_root_;
			for (std::size_t i = 0; i + 1 < segments.size(); ++i) {
				auto* child = table->get(segments[i]);
				if (!child || !child->is_table()) {
					table->insert_or_assign(segments[i], toml::table{});
					child = table->get(segments[i]);
				}
				auto* child_table = child ? child->as_table() : nullptr;
				if (!child_table) {
					table->insert_or_assign(segments[i], toml::table{});
					child = table->get(segments[i]);
					child_table = child ? child->as_table() : nullptr;
				}
				if (!child_table) {
					return;
				}
				table = child_table;
			}
			table->insert_or_assign(segments.back(), value);
		}

		std::optional<std::string> ApplicationConfig::TryGetStringUnlocked(const std::string& key) const {
			auto segments = SplitKeyPath(key);
			auto node = FindNodeUnlocked(segments);
			if (!node) return std::nullopt;

			if (auto str = node->value<std::string>()) return *str;
			if (auto boolean = node->value<bool>()) return *boolean ? "true" : "false";
			if (auto integer = node->value<int64_t>()) return std::to_string(*integer);
			if (auto floating = node->value<double>()) {
				std::ostringstream oss;
				oss << *floating;
				return oss.str();
			}
			return std::nullopt;
		}

		bool ApplicationConfig::TryMigrateFromLegacyIniUnlocked() {
			std::error_code ec;
			if (!std::filesystem::exists(legacy_config_file_path_, ec)) {
				return false;
			}
			try {
				pt::ptree legacy_tree;
				pt::read_ini(legacy_config_file_path_, legacy_tree);
				auto all_keys = config::Keys::GetAllKeys();
				for (const auto& key : all_keys) {
					auto opt = legacy_tree.get_optional<std::string>(key.data());
					if (opt.has_value()) {
						SetValueInternalUnlocked(std::string(key), *opt);
					}
				}
				LOG_INFO("Migrated legacy config file {} to TOML {}", legacy_config_file_path_, config_file_path_);
				return true;
			} catch (const std::exception& e) {
				LOG_ERR("Failed to migrate legacy ini: {}", e.what());
			}
			return false;
		}

		toml::node* ApplicationConfig::FindNodeUnlocked(const std::vector<std::string>& segments) {
			if (segments.empty()) return nullptr;
			toml::node* current = &toml_root_;
			for (const auto& segment : segments) {
				auto* current_table = current->as_table();
				if (!current_table) return nullptr;
				current = current_table->get(segment);
				if (!current) return nullptr;
			}
			return current;
		}

		const toml::node* ApplicationConfig::FindNodeUnlocked(const std::vector<std::string>& segments) const {
			if (segments.empty()) return nullptr;
			const toml::node* current = &toml_root_;
			for (const auto& segment : segments) {
				const auto* current_table = current->as_table();
				if (!current_table) return nullptr;
				current = current_table->get(segment);
				if (!current) return nullptr;
			}
			return current;
		}

		std::vector<std::string> ApplicationConfig::SplitKeyPath(const std::string& key) {
			std::vector<std::string> segments;
			std::string current;
			for (char ch : key) {
				if (ch == '.') {
					if (!current.empty()) {
						segments.emplace_back(current);
						current.clear();
					}
				} else {
					current.push_back(ch);
				}
			}
			if (!current.empty()) {
				segments.emplace_back(current);
			}
			return segments;
		}

		std::string ApplicationConfig::ResolveDefaultValue(const std::string& key_path, const std::string& def) const {
			if (key_path == config::Keys::Dir.get()) {
				std::error_code ec;
				if (def.empty() || !std::filesystem::exists(def, ec)) {
					return os::GetUserDownloadsDir();
				}
				return def;
			}
			if (key_path == config::Keys::ConfPath.get()) {
				std::error_code ec;
				std::string value = def;
				if (value.empty() || !std::filesystem::exists(value, ec)) {
					std::string conf_path;
#ifdef __APPLE__
					conf_path = os::GetExecutableDir() + "/../Resources/engine/aria2.conf";
#else
					conf_path = os::GetExecutableDir() + "/engine/aria2.conf";
#endif
					value = conf_path;
				}
				return value;
			}
			if (key_path == config::Keys::TrackerSourceUrls.get()) {
				return BuildTrackerJson(tracker_source_server_, /*use_name*/ false);
			}
			if (key_path == config::Keys::SaveSession.get()) {
				return EnsureSessionPath();
			}
			if (key_path == config::Keys::RpcSecret.get()) {
				return GenerateRpcSecret();
			}
			if (key_path == config::Keys::TrackerSourceNames.get()) {
				return DefaultTrackerSourceNamesJson();
			}
			return def;
		}

		std::optional<std::string> ApplicationConfig::ValidateExistingValue(const std::string& key_path,
																		   const std::string& current,
																		   const std::string& def) const {
			std::string cur = current;
			if (key_path == config::Keys::Dir.get()) {
				std::error_code ec;
				if (cur.empty() || !std::filesystem::exists(cur, ec)) {
					cur = os::GetUserDownloadsDir();
					return cur;
				}
				return std::nullopt;
			}
			if (key_path == config::Keys::ConfPath.get()) {
				std::error_code ec;
				if (cur.empty() || !std::filesystem::exists(cur, ec)) {
					std::string conf_path;
#ifdef __APPLE__
					conf_path = os::GetExecutableDir() + "/../Resources/engine/aria2.conf";
#else
					conf_path = os::GetExecutableDir() + "/engine/aria2.conf";
#endif
					return conf_path;
				}
				return std::nullopt;
			}
			if (key_path == config::Keys::TrackerSourceUrls.get() && cur.empty()) {
				return BuildTrackerJson(tracker_source_server_, /*use_name*/ false);
			}
			if (key_path == config::Keys::SaveSession.get() && cur.empty()) {
				return EnsureSessionPath();
			}
			if (key_path == config::Keys::RpcSecret.get() && (cur.empty() || cur == "GDownload_secret")) {
				return GenerateRpcSecret();
			}
			if (key_path == config::Keys::TrackerSourceNames.get() && cur.empty()) {
				return DefaultTrackerSourceNamesJson();
			}

			auto ldef = ToLower(def);
			if (ldef == "true" || ldef == "false") {
				bool bv{};
				bool ok = ParseBoolLike(cur, bv);
				if (!ok) {
					return ldef == "true" ? std::string("true") : std::string("false");
				}
				return std::nullopt;
			}
			if (IsIntegerString(def)) {
				if (!IsIntegerString(cur)) {
					return def;
				}
				return std::nullopt;
			}
			if (cur.empty() && !def.empty()) {
				return def;
			}
			return std::nullopt;
		}
	}  // namespace config
}  // namespace gdl

#include "config_ini.h"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "config_key.h"
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

			inline std::string BuildTrackerJson(const std::map<std::string, std::string>& source, bool use_name) {
				nlohmann::json json_data = nlohmann::json::array();
				for (const auto& item : source) json_data.push_back(use_name ? item.first : item.second);
				return json_data.dump();
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
		}  // namespace

		ApplicationConfig::~ApplicationConfig() {
			Save();
		}

		std::string ApplicationConfig::GetTrackerServerUrlByName(const std::string& name) {
			if (tracker_source_server_.find(name) != tracker_source_server_.end()) {
				return tracker_source_server_[name];
			}
			return "";
		}

		ApplicationConfig::ApplicationConfig() {
			config_file_path_ = os::GetAppDataDir() + "/gdownload/gd.toml";
			legacy_config_file_path_ = os::GetAppDataDir() + "/gdownload/gd.ini";
			/*
			 * ["ngosang-best-link","ngosang-best-mirror","ngosang-best-cdn","ngosang-all-link","ngosang-all-mirror","ngosang-all-cdn",
						"ngosang-all_udp-link","ngosang-all_udp-mirror","ngosang-all_udp-cdn",
						"ngosang-all_http-link","ngosang-all_http-mirror","ngosang-all_http-cdn",
						"ngosang-all_https-link","ngosang-all_https-mirror","ngosang-all_https-cdn",
						"XIU2-best-link","XIU2-best-cdn","XIU2-all-link","XIU2-all-cdn","XIU2-http-link","XIU2-http-cdn","XIU2-nohttp-link","XIU2-nohttp-cdn"]
			 */
			tracker_source_server_.insert(
				std::make_pair("ngosang-best-link",
							   "https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_best.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-best-mirror", "https://ngosang.github.io/trackerslist/trackers_best.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-best-cdn", "https://cdn.jsdelivr.net/gh/ngosang/trackerslist@master/trackers_best.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all-link", "https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all-mirror", "https://ngosang.github.io/trackerslist/trackers_all.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all-cdn", "https://cdn.jsdelivr.net/gh/ngosang/trackerslist@master/trackers_all.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all_udp-link",
							   "https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all_udp.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all_udp-mirror", "https://ngosang.github.io/trackerslist/trackers_all_udp.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all_udp-cdn", "https://cdn.jsdelivr.net/gh/ngosang/trackerslist@master/trackers_all_udp.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all_http-link",
							   "https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all_http.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all_http-mirror", "https://ngosang.github.io/trackerslist/trackers_all_http.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all_http-cdn",
							   "https://cdn.jsdelivr.net/gh/ngosang/trackerslist@master/trackers_all_http.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all_https-link",
							   "https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all_https.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all_https-mirror", "https://ngosang.github.io/trackerslist/trackers_all_https.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all_https-cdn",
							   "https://cdn.jsdelivr.net/gh/ngosang/trackerslist@master/trackers_all_https.txt"));
			tracker_source_server_.insert(std::make_pair("XIU2-best-link", "https://cf.trackerslist.com/best.txt"));
			tracker_source_server_.insert(
				std::make_pair("XIU2-best-cdn", "https://jsd.onmicrosoft.cn/gh/XIU2/TrackersListCollection/best.txt"));
			tracker_source_server_.insert(std::make_pair("XIU2-all-link", "https://cf.trackerslist.com/all.txt"));
			tracker_source_server_.insert(
				std::make_pair("XIU2-all-cdn", "https://jsd.onmicrosoft.cn/gh/XIU2/TrackersListCollection/all.txt"));
			tracker_source_server_.insert(std::make_pair("XIU2-http-link", "https://cf.trackerslist.com/http.txt"));
			tracker_source_server_.insert(
				std::make_pair("XIU2-http-cdn", "https://jsd.onmicrosoft.cn/gh/XIU2/TrackersListCollection/http.txt"));
			tracker_source_server_.insert(std::make_pair("XIU2-nohttp-link", "https://cf.trackerslist.com/nohttp.txt"));
			tracker_source_server_.insert(
				std::make_pair("XIU2-nohttp-cdn", "https://jsd.onmicrosoft.cn/gh/XIU2/TrackersListCollection/nohttp.txt"));

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
						toml_root_ = toml::parse_file(config_file_path_);
						parsed_toml = true;
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
				if (!EnsureConfigFileExists()) {
					return false;
				}
				std::ofstream config(config_file_path_, std::ios::trunc);
				if (!config.is_open()) {
					LOG_ERR("Failed to open config file for writing: {}", config_file_path_);
					return false;
				}
				config << toml_root_;
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
			if (key_path == config::Keys::TrackerSourceNames.get()) {
				return BuildTrackerJson(tracker_source_server_, /*use_name*/ true);
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
			if (key_path == config::Keys::TrackerSourceNames.get() && cur.empty()) {
				return BuildTrackerJson(tracker_source_server_, /*use_name*/ true);
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

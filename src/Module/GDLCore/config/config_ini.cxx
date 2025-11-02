#include "config_ini.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
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
			config_file_path_ = os::GetAppDataDir() + "/gdownload/gd.ini";
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
			tracker_source_server_.insert(std::make_pair(
				"ngosang-best-cdn", "https://cdn.jsdelivr.net/gh/ngosang/trackerslist@master/trackers_best.txt"));
			tracker_source_server_.insert(std::make_pair(
				"ngosang-all-link", "https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all-mirror", "https://ngosang.github.io/trackerslist/trackers_all.txt"));
			tracker_source_server_.insert(std::make_pair(
				"ngosang-all-cdn", "https://cdn.jsdelivr.net/gh/ngosang/trackerslist@master/trackers_all.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all_udp-link",
							   "https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all_udp.txt"));
			tracker_source_server_.insert(std::make_pair(
				"ngosang-all_udp-mirror", "https://ngosang.github.io/trackerslist/trackers_all_udp.txt"));
			tracker_source_server_.insert(std::make_pair(
				"ngosang-all_udp-cdn", "https://cdn.jsdelivr.net/gh/ngosang/trackerslist@master/trackers_all_udp.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all_http-link",
							   "https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all_http.txt"));
			tracker_source_server_.insert(std::make_pair(
				"ngosang-all_http-mirror", "https://ngosang.github.io/trackerslist/trackers_all_http.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all_http-cdn",
							   "https://cdn.jsdelivr.net/gh/ngosang/trackerslist@master/trackers_all_http.txt"));
			tracker_source_server_.insert(
				std::make_pair("ngosang-all_https-link",
							   "https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all_https.txt"));
			tracker_source_server_.insert(std::make_pair(
				"ngosang-all_https-mirror", "https://ngosang.github.io/trackerslist/trackers_all_https.txt"));
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
			tracker_source_server_.insert(std::make_pair(
				"XIU2-nohttp-cdn", "https://jsd.onmicrosoft.cn/gh/XIU2/TrackersListCollection/nohttp.txt"));

			EnsureConfigFileExists();
			Load();
		}

		bool ApplicationConfig::Load() {
			try {
				std::unique_lock lock(mutex_);
				pt::read_ini(config_file_path_, ptree_root_);
				auto all_paths	= config::Keys::GetAllKeys();
				auto all_values = config::Keys::GetAllValues();

				for (std::size_t i = 0; i < all_paths.size(); ++i) {
					auto key_path	= all_paths[i];
					std::string def = std::string(all_values[i].data());
					LOG_DBG("KEY {} VALUE {}", key_path, def);

					auto cur_opt = ptree_root_.get_optional<std::string>(key_path.data());
					if (!cur_opt.has_value()) {
						std::string value = def;
						if (key_path == config::Keys::Dir.get()) {
							std::error_code ec;
							if (value.empty() || !std::filesystem::exists(value, ec)) {
								value = os::GetUserDownloadsDir();
							}
						}
						else if (key_path == config::Keys::ConfPath.get()) {
							std::error_code ec;
							if (value.empty() || !std::filesystem::exists(value, ec)) {
								std::string conf_path;
#ifdef __APPLE__
								conf_path = os::GetExecutableDir() + "/../Resources/engine/aria2.conf";
#else
								conf_path = os::GetExecutableDir() + "/engine/aria2.conf";
#endif
								value = conf_path;
							}
						}
						else if (key_path == config::Keys::TrackerSourceUrls.get() && value.empty()) {
							nlohmann::json json_data = nlohmann::json::array();
							for (auto& item : tracker_source_server_)
								json_data.push_back(item.second);
							value = json_data.dump();
						}
						else if (key_path == config::Keys::SaveSession.get() && value.empty()) {
							value = os::GetAppDataDir() + "/gdownload/session/aria2.session";
							std::error_code ec;
							auto session_dir = std::filesystem::path(value).parent_path();
							if (!std::filesystem::exists(session_dir, ec)) {
								std::filesystem::create_directories(session_dir, ec);
							}
						}
						else if (key_path == config::Keys::TrackerSourceNames.get() && value.empty()) {
							nlohmann::json json_data = nlohmann::json::array();
							for (auto& item : tracker_source_server_)
								json_data.push_back(item.first);
							value = json_data.dump();
						}
						ptree_root_.put(key_path.data(), value);
					}
					else {
						std::string cur = *cur_opt;
						if (key_path == config::Keys::Dir.get()) {
							std::error_code ec;
							if (cur.empty() || !std::filesystem::exists(cur, ec)) {
								cur = os::GetUserDownloadsDir();
								ptree_root_.put(key_path.data(), cur);
							}
						}
						else if (key_path == config::Keys::ConfPath.get()) {
							std::error_code ec;
							if (cur.empty() || !std::filesystem::exists(cur, ec)) {
								std::string conf_path;
#ifdef __APPLE__
								conf_path = os::GetExecutableDir() + "/../Resources/engine/aria2.conf";
#else
								conf_path = os::GetExecutableDir() + "/engine/aria2.conf";
#endif
								cur = conf_path;
								ptree_root_.put(key_path.data(), cur);
							}
						}
						else if (key_path == config::Keys::TrackerSourceUrls.get() && cur.empty()) {
							nlohmann::json json_data = nlohmann::json::array();
							for (auto& item : tracker_source_server_)
								json_data.push_back(item.second);
							ptree_root_.put(key_path.data(), json_data.dump());
						}
						else if (key_path == config::Keys::SaveSession.get() && cur.empty()) {
							std::string value = os::GetAppDataDir() + "/gdownload/session/aria2.session";
							std::error_code ec;
							auto session_dir = std::filesystem::path(value).parent_path();
							if (!std::filesystem::exists(session_dir, ec)) {
								std::filesystem::create_directories(session_dir, ec);
							}
							ptree_root_.put(key_path.data(), value);
						}
						else if (key_path == config::Keys::TrackerSourceNames.get() && cur.empty()) {
							nlohmann::json json_data = nlohmann::json::array();
							for (auto& item : tracker_source_server_)
								json_data.push_back(item.first);
							ptree_root_.put(key_path.data(), json_data.dump());
						}
						else {
							// 类型驱动的默认值回填
							auto ldef	 = ToLower(def);
							bool updated = false;
							if (ldef == "true" || ldef == "false") {
								bool bv{};
								bool ok = ParseBoolLike(cur, bv);
								if (!ok) {
									ptree_root_.put(key_path.data(), ldef == "true" ? "true" : "false");
									updated = true;
								}
							}
							else if (IsIntegerString(def)) {
								if (!IsIntegerString(cur)) {
									ptree_root_.put(key_path.data(), def);
									updated = true;
								}
							}
							else {
								if (cur.empty() && !def.empty()) {
									ptree_root_.put(key_path.data(), def);
									updated = true;
								}
							}
							(void)updated;
						}
					}
				}
				lock.unlock();
				Save();
			} catch (std::exception& e) {
				LOG_ERR("Load ini fail error {}", e.what());
				return false;
			}
			return true;
		}

		bool ApplicationConfig::Save() {
			try {
				std::unique_lock lock(mutex_);
				EnsureConfigFileExists();
				pt::write_ini(config_file_path_, ptree_root_);
			} catch (std::exception& e) {
				LOG_ERR("save ini fail error {}", e.what());
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
					std::ofstream ini(config_file_path_);
					if (!ini.is_open()) {
						LOG_ERR("Failed to create config file: {}", config_file_path_);
						return false;
					}
					ini.close();
				} catch (const std::exception& e) {
					LOG_ERR("Failed to create config file: {}", e.what());
					return false;
				}
			}
			return true;
		}
	}  // namespace config
}  // namespace gdl

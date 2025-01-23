#include "config_ini.h"
#include <filesystem>
#include <fstream>
#include "config_key.h"
#include "logger.h"
#include "os/os.h"
namespace pt = boost::property_tree;
namespace gdl {
	namespace config {

		ApplicationConfig::~ApplicationConfig() {
			Save();
		}

		ApplicationConfig::ApplicationConfig() {
			config_file_path_ = os::GetAppDataDir() + "/gdownload/gd.ini";
			EnsureConfigFileExists();
			Load();
		}

		bool ApplicationConfig::Load() {
			try {
				std::unique_lock lock(mutex_);
				pt::read_ini(config_file_path_, ptree_root_);
				auto all_paths	= config::Keys::GetAllKeys();
				auto all_values = config::Keys::GetAllValues();
				for (auto i = 0; i < all_paths.size(); ++i) {
					auto key_path		= all_paths[i];
					std::string value	= all_values[i].data();
					auto child_optional = ptree_root_.get_optional<std::string>(key_path.data());
					if (!child_optional.has_value()) {
                        if (key_path == config::Keys::Dir.get()) {
							std::error_code ec;
							if (value.empty() || !std::filesystem::exists(value,ec)) {
								value = os::GetUserDownloadsDir();
							}
						}
                        else if (key_path == config::Keys::ConfPath.get()) {
							std::error_code ec;
							if (value.empty() || !std::filesystem::exists(value,ec)) {
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
							value = DEFAULT_TRACKER_SOURCE_URLS;
                        }
                        else if (key_path == config::Keys::SaveSession.get() && value.empty()) {
                            value = os::GetAppDataDir() + "/gdownload/session/aria2.session";
                            std::error_code ec;
                            const auto session_dir = std::filesystem::path(value).parent_path();
                            if (!std::filesystem::exists(session_dir, ec)) {
                                std::filesystem::create_directories(session_dir, ec);
                            }
                        }
						ptree_root_.put(key_path.data(), value.data());
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

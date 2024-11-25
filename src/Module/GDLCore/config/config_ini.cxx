#include "config_ini.h"
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
			Load();
		}

		bool ApplicationConfig::Load() {
			try {
				std::unique_lock lock(mutex_);
				pt::read_ini(config_file_path_, ptree_root_);
				auto all_paths	= config::Keys::GetAllKeys();
				auto all_values = config::Keys::GetAllValues();
				for (auto i = 0; i < all_paths.size(); ++i) {
					auto key_path = all_paths[i];
					auto value	  = all_values[i];
					if (ptree_root_.find(key_path.data()) == ptree_root_.not_found()) {
						if (key_path == "dir") {
							value = os::GetUserDownloadsDir();
						}
						ptree_root_.put(key_path.data(), value.data());
					}
				}
			} catch (std::exception& e) {
				LOG_ERR("Load ini fail error {}", e.what());
				return false;
			}
			return true;
		}

		bool ApplicationConfig::Save() {
			try {
				std::unique_lock lock(mutex_);
				pt::write_ini(config_file_path_, ptree_root_);
			} catch (std::exception& e) {
				LOG_ERR("save ini fail error {}", e.what());
				return false;
			}
			return true;
		}
	}  // namespace config
}  // namespace gdl

#include "config_ini.h"
#include "os/os.h"
namespace pt = boost::property_tree;
namespace gdl {
	namespace config {

		ApplicationConfig::~ApplicationConfig() {}

		ApplicationConfig::ApplicationConfig() {
			config_file_path_ = os::GetAppDataDir() + "/gdownload/gd.ini";
		}

		bool ApplicationConfig::Load() {
			try {
				pt::read_ini(config_file_path_, ptree_root_);

			} catch (...) {
				return false;
			}
			return true;
		}

		bool ApplicationConfig::Save() {
			try {
				pt::write_ini(config_file_path_, ptree_root_);
			} catch (...) {
				return false;
			}
			return true;
		}
	}  // namespace config
}  // namespace gdl

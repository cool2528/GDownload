#include "config.h"
#include "config_ini.h"
#include "logger.h"
namespace gdl {
	namespace config {

		void SetValue(const std::string& key, const ConfigValue& value) {
			ApplicationConfig::Instance().Put(key, value.AsString());
		}

		ConfigValue GetValue(const std::string& key, const ConfigValue& defaultValue) {
			auto opt = ApplicationConfig::Instance().TryGet<std::string>(key);
			if (!opt.has_value()) {
				return defaultValue;
			}
			return *opt;
		}

		 std::string GetTrackersServerUrl(const std::string& key) {
			return ApplicationConfig::Instance().GetTrackerServerUrlByName(key);
		}

	}  // namespace config
}  // namespace gdl

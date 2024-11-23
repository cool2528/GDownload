#include "config.h"
#include "config_ini.h"
#include "logger.h"
namespace gdl {
	namespace config {

		void SetValue(const std::string& key, const ConfigValue& value) {
			ApplicationConfig::Instance().Put(key, value.AsString());
		}

		ConfigValue GetValue(const std::string& key, const ConfigValue& defaultValue) {
			auto value = ApplicationConfig::Instance().Get<std::string>(key);
			if (value.empty()) {
				return defaultValue;
			}
			return value;
		}

	}  // namespace config
}  // namespace gdl

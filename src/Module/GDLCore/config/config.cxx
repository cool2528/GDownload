#include "config.h"
#include "config_ini.h"
#include "logger.h"
namespace gdl {
	namespace config {

		void SetValue(const std::string& key, const ConfigVariant& value) {
			try {
				if (std::holds_alternative<int>(value)) {
					auto v = std::get<int>(value);
					ApplicationConfig::Instance().Put(key, v);
				}
				else if (std::holds_alternative<std::string>(value)) {
					auto v = std::get<std::string>(value);
					ApplicationConfig::Instance().Put(key, v);
				}
				else if (std::holds_alternative<bool>(value)) {
					auto v = std::get<bool>(value);
					ApplicationConfig::Instance().Put(key, v);
				}
				else if (std::holds_alternative<double>(value)) {
					auto v = std::get<double>(value);
					ApplicationConfig::Instance().Put(key, v);
				}
			} catch (const std::bad_variant_access& ex) {
				LOG_ERR("{}", ex.what());
			}
		}

		ConfigVariant GetValue(const std::string& key, const ConfigVariant& defaultValue) {
			ConfigVariant res;
			try {
				std::visit(
					[&](auto&& arg) {
						using T = std::decay_t<decltype(arg)>;
						if constexpr (std::is_same_v<T, int>) {
							res = ApplicationConfig::Instance().Get<T>(key);
						}
						else if constexpr (std::is_same_v<T, std::string>) {
							res = ApplicationConfig::Instance().Get<T>(key);
						}
						else if constexpr (std::is_same_v<T, bool>) {
							res = ApplicationConfig::Instance().Get<T>(key);
						}
						else if constexpr (std::is_same_v<T, double>) {
							res = ApplicationConfig::Instance().Get<T>(key);
						}
						else {
							res = defaultValue;
						}
					},
					defaultValue);

			} catch (const std::bad_variant_access& ex) {
				LOG_ERR("{}", ex.what());
			}
			return res;
		}

	}  // namespace config
}  // namespace gdl

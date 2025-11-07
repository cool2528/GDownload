#pragma once
#include <algorithm>
#include <map>
#include <optional>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <vector>
#include <toml++/toml.h>
#include "globalTypes.h"
#include "singleton.hpp"
namespace gdl {
	namespace config {
		namespace detail {
			template <typename T>
			std::string ToStringValue(const T& value) {
				if constexpr (std::is_same_v<T, std::string>) {
					return value;
				} else if constexpr (std::is_same_v<T, const char*>) {
					return std::string(value);
				} else if constexpr (std::is_same_v<T, bool>) {
					return value ? "true" : "false";
				} else {
					return std::to_string(value);
				}
			}

			inline std::string ToLowerCopy(std::string value) {
				std::transform(value.begin(), value.end(), value.begin(),
							   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				return value;
			}

			template <typename T>
			std::optional<T> FromString(const std::string& value) {
				if constexpr (std::is_same_v<T, std::string>) {
					return value;
				} else if constexpr (std::is_same_v<T, bool>) {
					auto lower = ToLowerCopy(value);
					if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") return true;
					if (lower == "false" || lower == "0" || lower == "no" || lower == "off") return false;
					return std::nullopt;
				} else if constexpr (std::is_integral_v<T>) {
					try {
						long long parsed = std::stoll(value);
						return static_cast<T>(parsed);
					} catch (...) {
						return std::nullopt;
					}
				} else if constexpr (std::is_floating_point_v<T>) {
					try {
						double parsed = std::stod(value);
						return static_cast<T>(parsed);
					} catch (...) {
						return std::nullopt;
					}
				} else {
					return std::nullopt;
				}
			}
		}  // namespace detail

		class ApplicationConfig : public Singleton<ApplicationConfig> {
			SINGLETON_DECLARE(ApplicationConfig)
		   public:
			~ApplicationConfig();
			template <typename T>
			void Put(const std::string& key, const T& value) {
				std::unique_lock lock(mutex_);
				SetValueInternalUnlocked(key, detail::ToStringValue(value));
				lock.unlock();
				Save();
			}
			template <typename Type>
			Type Get(const std::string& key) {
				std::shared_lock lock(mutex_);
				auto optional_val = TryGetStringUnlocked(key);
				if (optional_val.has_value()) {
					auto converted = detail::FromString<Type>(*optional_val);
					if (converted.has_value()) return *converted;
				}
				return Type();
			}

			// 尝试获取配置值：存在则返回值，不存在返回 std::nullopt
			template <typename Type>
			std::optional<Type> TryGet(const std::string& key) {
				std::shared_lock lock(mutex_);
				auto optional_val = TryGetStringUnlocked(key);
				if (!optional_val.has_value()) return std::nullopt;
				return detail::FromString<Type>(*optional_val);
			}

			std::string GetTrackerServerUrlByName(const std::string& name);
		   private:
			explicit ApplicationConfig();
			bool Load();
			bool Save();
			bool EnsureConfigFileExists();
			void SetValueInternalUnlocked(const std::string& key, const std::string& value);
			std::optional<std::string> TryGetStringUnlocked(const std::string& key) const;
			bool TryMigrateFromLegacyIniUnlocked();
			toml::node* FindNodeUnlocked(const std::vector<std::string>& segments);
			const toml::node* FindNodeUnlocked(const std::vector<std::string>& segments) const;
			static std::vector<std::string> SplitKeyPath(const std::string& key);
			std::string ResolveDefaultValue(const std::string& key_path, const std::string& def) const;
			std::optional<std::string> ValidateExistingValue(const std::string& key_path,
															 const std::string& current,
															 const std::string& def) const;

		   private:
			toml::table toml_root_;
			String config_file_path_;
			String legacy_config_file_path_;
			std::shared_mutex mutex_;
			std::map<std::string,std::string> tracker_source_server_;
		};
	}  // namespace config
}  // namespace gdl

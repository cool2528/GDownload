#pragma once
#include <string>
#include <variant>
#include "config_key.h"
#include "export.h"
namespace gdl {
	namespace config {

		class ConfigValue {
		   public:
			ConfigValue() : value_("") {}
			ConfigValue(const std::string& v) : value_(v) {}
			ConfigValue(const char* v) : value_(v) {}
			ConfigValue(int v) : value_(std::to_string(v)) {}
			ConfigValue(bool v) : value_(v ? "true" : "false") {}
			ConfigValue(double v) : value_(std::to_string(v)) {}

			std::string AsString() const { return value_; }

			int AsInt() const {
				try {
					return std::stoi(value_);
				} catch (...) {
					return 0;
				}
			}

			bool AsBool() const { return value_ == "true" || value_ == "1"; }

			double AsDouble() const {
				try {
					return std::stod(value_);
				} catch (...) {
					return 0.0;
				}
			}
			bool IsEmpty() const { return value_.empty(); }
			operator bool() const { return AsBool(); }
			operator std::string() const { return AsString(); }
			operator double() const { return AsDouble(); }
			operator int() const { return AsInt(); }
			ConfigValue& operator=(const std::string& v) {
				value_ = v;
				return *this;
			}
			ConfigValue& operator=(int v) {
				value_ = std::to_string(v);
				return *this;
			}
			ConfigValue& operator=(bool v) {
				value_ = v ? "true" : "false";
				return *this;
			}
			ConfigValue& operator=(double v) {
				value_ = std::to_string(v);
				return *this;
			}

			bool operator==(const ConfigValue& other) const { return value_ == other.value_; }
			bool operator!=(const ConfigValue& other) const { return !(*this == other); }

		   private:
			std::string value_;
		};
		GDLCore_API void SetValue(const std::string& key, const ConfigValue& value);
		GDLCore_API ConfigValue GetValue(const std::string& key, const ConfigValue& defaultValue = std::string(""));
		GDLCore_API std::string GetTrackersServerUrl(const std::string& key);
	}  // namespace config
}  // namespace gdl

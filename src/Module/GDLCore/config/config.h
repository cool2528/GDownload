#pragma once
#include <string>
#include <variant>
#include "export.h"

namespace gdl {
	namespace config {
		using ConfigVariant = std::variant<int, bool, std::string, double>;
		GDLCore_API void SetValue(const std::string& key, const ConfigVariant& value);
		GDLCore_API ConfigVariant GetValue(const std::string& key, const ConfigVariant& defaultValue = "");
	}  // namespace config
}  // namespace gdl

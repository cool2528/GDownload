#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace gdl {
#ifdef _WIN32
	using String	  = std::string;
	using String_View = std::string_view;
#else
	using String	  = std::string;
	using String_View = std::string_view;
#endif
}  // namespace gdl

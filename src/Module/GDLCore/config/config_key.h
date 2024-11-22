#pragma once
namespace gdl {
	namespace config {
		struct ConfigPath {
			const char* path;
			constexpr ConfigPath(const char* p) : path(p) {}
			operator const char*() const { return path; }
		};

#define CONFIG_PATH(name, path)        \
	static constexpr ConfigPath name { \
		path                           \
	}

		struct Keys {
			CONFIG_PATH(WindowSize, "general.window_size");
			CONFIG_PATH(Theme, "general.theme");
			CONFIG_PATH(Language, "general.language");
		};
	}  // namespace config
}  // namespace gdl

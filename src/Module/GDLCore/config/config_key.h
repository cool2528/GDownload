#pragma once
#include <array>
#include <string>
namespace gdl {
	namespace config {
		template <std::size_t N>
		struct ConfigKeys {
			constexpr ConfigKeys(const std::array<std::string_view, N>& keys) : keys_(keys) {}

			constexpr auto begin() const { return keys_.begin(); }
			constexpr auto end() const { return keys_.end(); }
			constexpr std::size_t size() const { return N; }

			const std::array<std::string_view, N> keys_;
		};

		struct ConfigPath {
			const char* path;
			constexpr ConfigPath(const char* p) : path(p) {}
			constexpr operator const char*() const { return path; }
			constexpr operator std::string() const { return std::string(path); }
			constexpr std::string_view get() const { return path; }
		};

#define CONFIG_PATH(name, path)        \
	static constexpr ConfigPath name { \
		path                           \
	}

		struct Keys {
			CONFIG_PATH(WindowSize, "general.window_size");
			CONFIG_PATH(Theme, "general.theme");
			CONFIG_PATH(Language, "general.language");
			CONFIG_PATH(BtExludeTracker, "aria2c.bt-exclude-tracker");
			CONFIG_PATH(BtTracker, "aria2c.bt-tracker");
			CONFIG_PATH(Dir, "aria2c.dir");
			CONFIG_PATH(ListenPort, "aria2c.listen-port");
			CONFIG_PATH(RpcListenPort, "aria2c.rpc-listen-port");
			CONFIG_PATH(Split, "aria2c.split");
			CONFIG_PATH(UserAgent, "aria2c.user-agent");
			CONFIG_PATH(AllProxy, "aria2c.all-proxy");
			CONFIG_PATH(DhtListenPort, "aria2c.dht-listen-port");
			CONFIG_PATH(MaxConcurrentDownloads, "aria2c.max-concurrent-downloads");

			// static function
			static constexpr auto GetAllKeys() {
				return ConfigKeys(std::array{WindowSize.get(), Theme.get(), Language.get(), BtExludeTracker.get(),
											 Dir.get(), ListenPort.get(), RpcListenPort.get(), Split.get(),
											 UserAgent.get(), AllProxy.get(), DhtListenPort.get(),
											 MaxConcurrentDownloads.get()});
			}
		};
	}  // namespace config
}  // namespace gdl

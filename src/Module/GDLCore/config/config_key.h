#pragma once
#include <array>
#include <string>
#define DEFAULT_TRACKER_SOURCE_URLS                                                                                 \
	"[\"https://ngosang.github.io/trackerslist/trackers_best.txt\",\"https://ngosang.github.io/trackerslist/"       \
	"trackers_all.txt\",\"https://ngosang/trackerslist/master/trackers_all_http.txt\",\"https://ngosang.github.io/" \
	"trackerslist/trackers_all_https.txt\"]"
namespace gdl {
	namespace config {
		template <std::size_t N>
		struct ConfigKeys {
			constexpr ConfigKeys(const std::array<std::string_view, N>& keys) : keys_(keys) {}

			constexpr auto begin() const { return keys_.begin(); }
			constexpr auto end() const { return keys_.end(); }
			constexpr std::size_t size() const { return N; }
			constexpr auto operator[](std::size_t i) const { return keys_.at(i); }
			const std::array<std::string_view, N> keys_;
		};

		struct ConfigPath {
			const char* path;
			const char* value;
			constexpr ConfigPath(const char* p, const char* v) : path(p), value(v) {}
			constexpr operator const char*() const { return path; }
			constexpr operator std::string() const { return std::string(path); }
			constexpr std::string_view get() const { return path; }
			constexpr std::string_view val() const { return value; }
		};

#define CONFIG_PATH(name, path, val)   \
	static constexpr ConfigPath name { \
		path, val                      \
	}

		struct Keys {
			CONFIG_PATH(WindowSize, "general.window_size", "1024,768");
			CONFIG_PATH(Theme, "general.theme", "Light");
			CONFIG_PATH(Language, "general.language", "zh-cn");
			CONFIG_PATH(BtExludeTracker, "aria2c.bt-exclude-tracker", "");
			CONFIG_PATH(BtTracker, "aria2c.bt-tracker", "");
			CONFIG_PATH(Dir, "aria2c.dir", "");
			CONFIG_PATH(ListenPort, "aria2c.listen-port", "21301");
			CONFIG_PATH(RpcListenPort, "aria2c.rpc-listen-port", "");
			CONFIG_PATH(Split, "aria2c.split", "");
			CONFIG_PATH(UserAgent, "aria2c.user-agent", "");
			CONFIG_PATH(AllProxy, "aria2c.all-proxy", "");
			CONFIG_PATH(DhtListenPort, "aria2c.dht-listen-port", "26701");
			CONFIG_PATH(MaxConcurrentDownloads, "aria2c.max-concurrent-downloads", "64");
			CONFIG_PATH(ConfPath, "aria2c.conf-path", "");
			CONFIG_PATH(TrackerSourceUrls, "aria2c.tracker_source_urls", DEFAULT_TRACKER_SOURCE_URLS);
			// static function all keys
			static constexpr auto GetAllKeys() {
				return ConfigKeys(std::array{WindowSize.get(), Theme.get(), Language.get(), BtExludeTracker.get(),
                                             BtTracker.get(), Dir.get(), ListenPort.get(), RpcListenPort.get(),
                                             Split.get(), UserAgent.get(), AllProxy.get(), DhtListenPort.get(),
											 MaxConcurrentDownloads.get(), ConfPath.get(), TrackerSourceUrls.get()});
			}
			// static function all values
			static constexpr auto GetAllValues() {
				return ConfigKeys(std::array{WindowSize.val(), Theme.val(), Language.val(), BtExludeTracker.val(),
                                             BtTracker.val(), Dir.val(), ListenPort.val(), RpcListenPort.val(),
                                             Split.val(), UserAgent.val(), AllProxy.val(), DhtListenPort.val(),
											 MaxConcurrentDownloads.val(), ConfPath.val(), TrackerSourceUrls.val()});
			}
		};
	}  // namespace config
}  // namespace gdl

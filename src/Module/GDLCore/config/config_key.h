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
            CONFIG_PATH(Language, "general.language", "en_US");
			CONFIG_PATH(BtExludeTracker, "aria2c.bt-exclude-tracker", "");
			CONFIG_PATH(BtTracker, "aria2c.bt-tracker", "");
			CONFIG_PATH(Dir, "aria2c.dir", "");
			CONFIG_PATH(ListenPort, "aria2c.listen-port", "21301");
			CONFIG_PATH(RpcListenPort, "aria2c.rpc-listen-port", "16888");
			CONFIG_PATH(RpcSecret, "aria2c.rpc-secret", "GDownload_secret");
			CONFIG_PATH(Split, "aria2c.split", "");
			CONFIG_PATH(UserAgent, "aria2c.user-agent", "");
			CONFIG_PATH(AllProxy, "aria2c.all-proxy", "");
			CONFIG_PATH(DhtListenPort, "aria2c.dht-listen-port", "26701");
			CONFIG_PATH(MaxConcurrentDownloads, "aria2c.max-concurrent-downloads", "64");
			CONFIG_PATH(ConfPath, "aria2c.conf-path", "");
			CONFIG_PATH(TrackerSourceUrls, "aria2c.tracker_source_urls", "");
            CONFIG_PATH(SaveSession, "aria2c.save-session", "");
            CONFIG_PATH(IsSaveSession, "aria2c.is-save-session", "true");
            CONFIG_PATH(EnableGlobalProxy, "aria2c.enable-global-proxy", "false");
            CONFIG_PATH(GlobalProxy, "aria2c.global-proxy", "");
            CONFIG_PATH(ListenClipboard, "general.listen-clipboard", "true");
            CONFIG_PATH(AutoResumeTask, "aria2c.auto-resume-task", "true");
            CONFIG_PATH(AutoStart, "general.auto-start", "false");
            CONFIG_PATH(RememberWindowPosition, "general.remember-window-position", "true");
            CONFIG_PATH(EnableTrayIcon, "general.enable-tray-icon", "true");
            CONFIG_PATH(EnableNotification, "general.enable-notification", "true");
            CONFIG_PATH(EnableAutoShutdown, "general.enable-auto-shutdown", "false");
            CONFIG_PATH(EnableAutoUpdate, "general.enable-auto-update", "true");
            CONFIG_PATH(WindowPosition, "general.window-position", "");
            CONFIG_PATH(BaiduPanCookies, "plugin.baidu-pan-cookies", "");
			CONFIG_PATH(TrackerSourceNames,"aria2c.tracker_source_name","");
			CONFIG_PATH(EnableTrackerSourceAutoUpdate,"aria2c.enable_tracker_source_auto_update","true");
			CONFIG_PATH(ShowCloseConfirm, "general.show-close-confirm", "true");
			CONFIG_PATH(CloseToTray, "general.close-to-tray", "false");
			// 速度控制配置
			CONFIG_PATH(MaxDownloadLimit, "aria2c.max-download-limit", "0");
			CONFIG_PATH(MaxOverallDownloadLimit, "aria2c.max-overall-download-limit", "0");
			CONFIG_PATH(MaxUploadLimit, "aria2c.max-upload-limit", "0");
			CONFIG_PATH(MaxOverallUploadLimit, "aria2c.max-overall-upload-limit", "0");
			CONFIG_PATH(LowestSpeedLimit, "aria2c.lowest-speed-limit", "0");


			// static function all keys
			static constexpr auto GetAllKeys() {
                return ConfigKeys(std::array{WindowSize.get(),
                                             Theme.get(),
                                             Language.get(),
                                             BtExludeTracker.get(),
                                             BtTracker.get(),
                                             Dir.get(),
                                             ListenPort.get(),
                                             RpcListenPort.get(),
                                             RpcSecret.get(),
                                             Split.get(),
                                             UserAgent.get(),
                                             AllProxy.get(),
                                             DhtListenPort.get(),
                                             MaxConcurrentDownloads.get(),
                                             ConfPath.get(),
                                             TrackerSourceUrls.get(),
                                             SaveSession.get(),
                                             IsSaveSession.get(),
                                             EnableGlobalProxy.get(),
                                             GlobalProxy.get(),
                                             ListenClipboard.get(),
                                             AutoResumeTask.get(),
                                             AutoStart.get(),
                                             RememberWindowPosition.get(),
                                             EnableTrayIcon.get(),
                                             EnableNotification.get(),
                                             EnableAutoShutdown.get(),
                                             EnableAutoUpdate.get(),
                                             WindowPosition.get(),
                                             BaiduPanCookies.get(),
									         TrackerSourceNames.get(),
											 EnableTrackerSourceAutoUpdate.get(),
											 ShowCloseConfirm.get(),
											 CloseToTray.get(),
											 MaxDownloadLimit.get(),
											 MaxOverallDownloadLimit.get(),
											 MaxUploadLimit.get(),
											 MaxOverallUploadLimit.get(),
											 LowestSpeedLimit.get()});
			}
			// static function all values
			static constexpr auto GetAllValues() {
				return ConfigKeys(std::array{WindowSize.val(),
											 Theme.val(),
											 Language.val(),
											 BtExludeTracker.val(),
											 BtTracker.val(),
											 Dir.val(),
											 ListenPort.val(),
											 RpcListenPort.val(),
											 RpcSecret.val(),
											 Split.val(),
											 UserAgent.val(),
											 AllProxy.val(),
											 DhtListenPort.val(),
											 MaxConcurrentDownloads.val(),
											 ConfPath.val(),
											 TrackerSourceUrls.val(),
											 SaveSession.val(),
											 IsSaveSession.val(),
											 EnableGlobalProxy.val(),
											 GlobalProxy.val(),
											 ListenClipboard.val(),
											 AutoResumeTask.val(),
											 AutoStart.val(),
											 RememberWindowPosition.val(),
											 EnableTrayIcon.val(),
											 EnableNotification.val(),
											 EnableAutoShutdown.val(),
											 EnableAutoUpdate.val(),
                                             WindowPosition.val(),
                                             BaiduPanCookies.val(),
											 TrackerSourceNames.val(),
											 EnableTrackerSourceAutoUpdate.val(),
											 ShowCloseConfirm.val(),
											 CloseToTray.val(),
											 MaxDownloadLimit.val(),
											 MaxOverallDownloadLimit.val(),
											 MaxUploadLimit.val(),
											 MaxOverallUploadLimit.val(),
											 LowestSpeedLimit.val()});
			}
		};
	}  // namespace config
}  // namespace gdl

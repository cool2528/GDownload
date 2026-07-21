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
            CONFIG_PATH(Language, "general.language", "zh_CN");
			CONFIG_PATH(BtExludeTracker, "aria2c.bt-exclude-tracker", "");
			CONFIG_PATH(BtTracker, "aria2c.bt-tracker", "");
			CONFIG_PATH(Dir, "aria2c.dir", "");
			CONFIG_PATH(ListenPort, "aria2c.listen-port", "21301");
			CONFIG_PATH(RpcListenPort, "aria2c.rpc-listen-port", "16888");
			CONFIG_PATH(RpcSecret, "aria2c.rpc-secret", "");
			CONFIG_PATH(Split, "aria2c.split", "64");
			CONFIG_PATH(UserAgent, "aria2c.user-agent", "");
			CONFIG_PATH(AllProxy, "aria2c.all-proxy", "");
			CONFIG_PATH(DhtListenPort, "aria2c.dht-listen-port", "6881");
			CONFIG_PATH(MaxConcurrentDownloads, "aria2c.max-concurrent-downloads", "5");
			CONFIG_PATH(ConfPath, "aria2c.conf-path", "");
			CONFIG_PATH(TrackerSourceUrls, "aria2c.tracker_source_urls", "");
            CONFIG_PATH(SaveSession, "aria2c.save-session", "");
            CONFIG_PATH(IsSaveSession, "aria2c.is-save-session", "false");
            CONFIG_PATH(EnableGlobalProxy, "aria2c.enable-global-proxy", "false");
            CONFIG_PATH(GlobalProxy, "aria2c.global-proxy", "");
            CONFIG_PATH(ListenClipboard, "general.listen-clipboard", "true");
            CONFIG_PATH(AutoResumeTask, "aria2c.auto-resume-task", "false");
            CONFIG_PATH(AutoStart, "general.auto-start", "false");
            CONFIG_PATH(RememberWindowPosition, "general.remember-window-position", "true");
            CONFIG_PATH(EnableTrayIcon, "general.enable-tray-icon", "true");
            CONFIG_PATH(EnableNotification, "general.enable-notification", "true");
            CONFIG_PATH(EnableAutoShutdown, "general.enable-auto-shutdown", "false");
            CONFIG_PATH(EnableAutoUpdate, "general.enable-auto-update", "true");
            CONFIG_PATH(EnableGithubAccelerate, "general.enable-github-accelerate", "false");
            CONFIG_PATH(WindowPosition, "general.window-position", "");
            CONFIG_PATH(BaiduPanCookies, "plugin.baidu-pan-cookies", "");
            CONFIG_PATH(PluginSourceProxy, "plugin.source-proxy", "");
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
			// 连接和性能参数配置
			CONFIG_PATH(MaxConnectionPerServer, "aria2c.max-connection-per-server", "16");
			CONFIG_PATH(MinSplitSize, "aria2c.min-split-size", "20");  // 单位：MB
			// 下载完成后操作配置
			CONFIG_PATH(OnCompleteAction, "aria2c.on-complete-action", "0");  // 0=无操作, 1=打开文件, 2=打开目录, 3=播放声音, 4=自定义命令, 5=关机, 6=睡眠, 7=重启
			CONFIG_PATH(CustomCompleteCommand, "aria2c.custom-complete-command", "");
			CONFIG_PATH(OnErrorAction, "aria2c.on-error-action", "0");  // 0=无操作, 1=播放声音, 2=自定义命令
			CONFIG_PATH(CustomErrorCommand, "aria2c.custom-error-command", "");
			CONFIG_PATH(OnStartAction, "aria2c.on-start-action", "0");  // 0=无操作, 1=播放声音
			// 超时和重试配置
			CONFIG_PATH(Timeout, "aria2c.timeout", "60");
			CONFIG_PATH(ConnectTimeout, "aria2c.connect-timeout", "60");
			CONFIG_PATH(MaxTries, "aria2c.max-tries", "5");
			CONFIG_PATH(RetryWait, "aria2c.retry-wait", "0");
			// BitTorrent 高级配置
			CONFIG_PATH(EnableDht, "aria2c.enable-dht", "true");
			CONFIG_PATH(BtMaxPeers, "aria2c.bt-max-peers", "55");
			CONFIG_PATH(BtRequireCrypto, "aria2c.bt-require-crypto", "false");

			// eD2k 引擎设置组
			CONFIG_PATH(Ed2kNickname, "ed2k.nickname", "GDownload");
			CONFIG_PATH(Ed2kTcpPort, "ed2k.tcp-port", "4662");
			CONFIG_PATH(Ed2kUdpPort, "ed2k.udp-port", "4672");
			CONFIG_PATH(Ed2kEnableKad, "ed2k.enable-kad", "false");
			CONFIG_PATH(Ed2kEnableObfuscation, "ed2k.enable-obfuscation", "false");
			CONFIG_PATH(Ed2kAutoConnect, "ed2k.auto-connect", "true");
			CONFIG_PATH(Ed2kMaxConcurrentTasks, "ed2k.max-concurrent-tasks", "5");
			CONFIG_PATH(Ed2kSharedDirs, "ed2k.shared-dirs", "");
			CONFIG_PATH(Ed2kServerMetUrl, "ed2k.server-met-url", "http://upd.emule-security.org/server.met");
			CONFIG_PATH(Ed2kNodesDatUrl, "ed2k.nodes-dat-url", "http://upd.emule-security.org/nodes.dat");

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
                                             EnableGithubAccelerate.get(),
                                             WindowPosition.get(),
                                             BaiduPanCookies.get(),
                                             PluginSourceProxy.get(),
									         TrackerSourceNames.get(),
											 EnableTrackerSourceAutoUpdate.get(),
											 ShowCloseConfirm.get(),
											 CloseToTray.get(),
											 MaxDownloadLimit.get(),
											 MaxOverallDownloadLimit.get(),
											 MaxUploadLimit.get(),
											 MaxOverallUploadLimit.get(),
											 LowestSpeedLimit.get(),
											 MaxConnectionPerServer.get(),
											 MinSplitSize.get(),
											 OnCompleteAction.get(),
											 CustomCompleteCommand.get(),
											 OnErrorAction.get(),
											 CustomErrorCommand.get(),
											 OnStartAction.get(),
											 Timeout.get(),
											 ConnectTimeout.get(),
											 MaxTries.get(),
											 RetryWait.get(),
											 EnableDht.get(),
											 BtMaxPeers.get(),
											 BtRequireCrypto.get(),
											 Ed2kNickname.get(),
											 Ed2kTcpPort.get(),
											 Ed2kUdpPort.get(),
											 Ed2kEnableKad.get(),
											 Ed2kEnableObfuscation.get(),
											 Ed2kAutoConnect.get(),
											 Ed2kMaxConcurrentTasks.get(),
											 Ed2kSharedDirs.get(),
											 Ed2kServerMetUrl.get(),
											 Ed2kNodesDatUrl.get()});
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
                                             EnableGithubAccelerate.val(),
                                             WindowPosition.val(),
                                             BaiduPanCookies.val(),
                                             PluginSourceProxy.val(),
											 TrackerSourceNames.val(),
											 EnableTrackerSourceAutoUpdate.val(),
											 ShowCloseConfirm.val(),
											 CloseToTray.val(),
											 MaxDownloadLimit.val(),
											 MaxOverallDownloadLimit.val(),
											 MaxUploadLimit.val(),
											 MaxOverallUploadLimit.val(),
											 LowestSpeedLimit.val(),
											 MaxConnectionPerServer.val(),
											 MinSplitSize.val(),
											 OnCompleteAction.val(),
											 CustomCompleteCommand.val(),
											 OnErrorAction.val(),
											 CustomErrorCommand.val(),
											 OnStartAction.val(),
											 Timeout.val(),
											 ConnectTimeout.val(),
											 MaxTries.val(),
											 RetryWait.val(),
											 EnableDht.val(),
											 BtMaxPeers.val(),
											 BtRequireCrypto.val(),
											 Ed2kNickname.val(),
											 Ed2kTcpPort.val(),
											 Ed2kUdpPort.val(),
											 Ed2kEnableKad.val(),
											 Ed2kEnableObfuscation.val(),
											 Ed2kAutoConnect.val(),
											 Ed2kMaxConcurrentTasks.val(),
											 Ed2kSharedDirs.val(),
											 Ed2kServerMetUrl.val(),
											 Ed2kNodesDatUrl.val()});
			}
		};
	}  // namespace config
}  // namespace gdl

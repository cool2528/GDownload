#pragma once
#include <string>

namespace gd {
	namespace ui {
		// Native Messaging host 自注册：应用启动时写入 host manifest（含固定扩展 ID 的
		// allowed_origins/allowed_extensions）并注册到浏览器约定的位置，使浏览器扩展的
		// chrome.runtime.connectNative('com.gdownload.host') 能发现并启动 host。
		// 采用 per-user（Windows HKCU / mac、linux 用户目录），无需管理员权限。
		class NativeHostRegistrar {
		   public:
			// 确保已注册（幂等）。返回是否至少一个浏览器注册成功。
			static bool EnsureRegistered();

			// 卸载清理：移除已写入的注册表键/文件（供卸载器或重置调用）。
			static bool Unregister();

		   private:
			// host 可执行文件的绝对路径（与主程序同目录）
			static std::string HostExecutablePath();
			// host manifest 存放目录（AppData/gdownload/native-host）
			static std::string ManifestDir();
		};
	}  // namespace ui
}  // namespace gd

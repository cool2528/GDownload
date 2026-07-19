#include "native_host_registrar.h"

#include <filesystem>
#include <fstream>
#include <vector>

#include "GDLCore/logger.h"
#include "os/os.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace gd {
	namespace ui {

		namespace {
			// Native Messaging host 名称（与扩展 nativeBridge.ts 的 HOST_NAME 一致）
			constexpr const char* kHostName = "com.gdownload.host";
			// 固定扩展 ID（由固定打包私钥决定，见 browser-extension/PACKAGING.md）
			constexpr const char* kChromeExtensionId = "kllgnmkbgmlefbmliedjnfffbifelcmb";
			constexpr const char* kFirefoxExtensionId = "gdownload@cool2528.github.io";

#ifdef _WIN32
			std::wstring Utf8ToWide(const std::string& s) {
				if (s.empty()) return std::wstring();
				const int n = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
				std::wstring w(static_cast<size_t>(n), L'\0');
				::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
				return w;
			}

			// 写注册表默认值（REG_SZ）为字符串
			bool WriteRegistryDefault(HKEY root, const std::wstring& subkey, const std::wstring& value) {
				HKEY hkey = nullptr;
				if (::RegCreateKeyExW(root, subkey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hkey, nullptr) !=
					ERROR_SUCCESS) {
					return false;
				}
				const auto bytes = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
				const LONG r = ::RegSetValueExW(hkey, nullptr, 0, REG_SZ,
												reinterpret_cast<const BYTE*>(value.c_str()), bytes);
				::RegCloseKey(hkey);
				return r == ERROR_SUCCESS;
			}

			void DeleteRegistryKey(HKEY root, const std::wstring& subkey) {
				::RegDeleteKeyExW(root, subkey.c_str(), KEY_WOW64_64KEY, 0);
			}
#endif

			bool WriteTextFile(const std::string& path, const std::string& content) {
				std::ofstream out(path, std::ios::binary | std::ios::trunc);
				if (!out.is_open()) return false;
				out.write(content.data(), static_cast<std::streamsize>(content.size()));
				return out.good();
			}

			// 创建父目录并写入 manifest（跨平台）。用于 mac/linux 的 per-browser 注册。
			bool WriteManifestTo(const std::string& path, const std::string& content) {
				std::error_code ec;
				std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
				return WriteTextFile(path, content);
			}

			// 生成 host manifest JSON（path 用正斜杠，Windows 文件 API 兼容）
			std::string BuildManifestJson(const std::string& host_exe, const std::string& allowed_key,
										  const std::string& allowed_value) {
				std::string json;
				json += "{\n";
				json += "  \"name\": \"" + std::string(kHostName) + "\",\n";
				json += "  \"description\": \"GDownload browser extension native messaging host\",\n";
				json += "  \"path\": \"" + host_exe + "\",\n";
				json += "  \"type\": \"stdio\",\n";
				json += "  \"" + allowed_key + "\": [\"" + allowed_value + "\"]\n";
				json += "}\n";
				return json;
			}
		}  // namespace

		std::string NativeHostRegistrar::HostExecutablePath() {
			// GetExecutableDir 返回正斜杠路径；mac/linux host 无 .exe 后缀
#ifdef _WIN32
			return gdl::os::GetExecutableDir() + "/gdownload_native_host.exe";
#else
			return gdl::os::GetExecutableDir() + "/gdownload_native_host";
#endif
		}

		std::string NativeHostRegistrar::ManifestDir() {
			return gdl::os::GetAppDataDir() + "/gdownload/native-host";
		}

		bool NativeHostRegistrar::EnsureRegistered() {
			const std::string host_exe = HostExecutablePath();
			// Chrome/Edge/Chromium 系用 allowed_origins；Firefox 用 allowed_extensions
			const std::string chrome_json = BuildManifestJson(
				host_exe, "allowed_origins", "chrome-extension://" + std::string(kChromeExtensionId) + "/");
			const std::string firefox_json =
				BuildManifestJson(host_exe, "allowed_extensions", kFirefoxExtensionId);

#ifdef _WIN32
			// Windows：manifest 写 AppData，注册表键指向它
			const std::string dir = ManifestDir();
			std::error_code ec;
			std::filesystem::create_directories(dir, ec);
			const std::string chrome_manifest = dir + "/com.gdownload.host.chrome.json";
			const std::string firefox_manifest = dir + "/com.gdownload.host.firefox.json";
			WriteTextFile(chrome_manifest, chrome_json);
			WriteTextFile(firefox_manifest, firefox_json);

			const std::wstring chrome_path_w = Utf8ToWide(chrome_manifest);
			const std::wstring firefox_path_w = Utf8ToWide(firefox_manifest);
			bool any = false;
			any |= WriteRegistryDefault(HKEY_CURRENT_USER,
										L"Software\\Google\\Chrome\\NativeMessagingHosts\\com.gdownload.host",
										chrome_path_w);
			any |= WriteRegistryDefault(HKEY_CURRENT_USER,
										L"Software\\Microsoft\\Edge\\NativeMessagingHosts\\com.gdownload.host",
										chrome_path_w);
			any |= WriteRegistryDefault(HKEY_CURRENT_USER,
										L"Software\\Mozilla\\NativeMessagingHosts\\com.gdownload.host",
										firefox_path_w);
#elif defined(__APPLE__)
			// macOS：manifest 直接放入各浏览器的 NativeMessagingHosts 目录（文件即注册）
			const char* home_env = std::getenv("HOME");
			const std::string home = home_env ? home_env : "";
			const std::string support = home + "/Library/Application Support";
			bool any = false;
			any |= WriteManifestTo(support + "/Google/Chrome/NativeMessagingHosts/com.gdownload.host.json", chrome_json);
			any |= WriteManifestTo(support + "/Microsoft Edge/NativeMessagingHosts/com.gdownload.host.json", chrome_json);
			any |= WriteManifestTo(support + "/Mozilla/NativeMessagingHosts/com.gdownload.host.json", firefox_json);
#else
			// Linux：Chromium 系用 ~/.config/<browser>/NativeMessagingHosts，Firefox 用 ~/.mozilla/native-messaging-hosts
			const char* home_env = std::getenv("HOME");
			const std::string home = home_env ? home_env : "";
			const char* xdg = std::getenv("XDG_CONFIG_HOME");
			const std::string config = (xdg && xdg[0] == '/') ? std::string(xdg) : home + "/.config";
			bool any = false;
			any |= WriteManifestTo(config + "/google-chrome/NativeMessagingHosts/com.gdownload.host.json", chrome_json);
			any |= WriteManifestTo(config + "/microsoft-edge/NativeMessagingHosts/com.gdownload.host.json", chrome_json);
			any |= WriteManifestTo(config + "/chromium/NativeMessagingHosts/com.gdownload.host.json", chrome_json);
			any |= WriteManifestTo(home + "/.mozilla/native-messaging-hosts/com.gdownload.host.json", firefox_json);
#endif
			if (any) {
				LOG_INFO("native messaging host registered (host: {})", host_exe);
			} else {
				LOG_ERR("native messaging host registration failed for all browsers");
			}
			return any;
		}

		bool NativeHostRegistrar::Unregister() {
#ifdef _WIN32
			DeleteRegistryKey(HKEY_CURRENT_USER, L"Software\\Google\\Chrome\\NativeMessagingHosts\\com.gdownload.host");
			DeleteRegistryKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Edge\\NativeMessagingHosts\\com.gdownload.host");
			DeleteRegistryKey(HKEY_CURRENT_USER, L"Software\\Mozilla\\NativeMessagingHosts\\com.gdownload.host");
			return true;
#else
			// mac/linux：删除各浏览器目录下的 manifest 文件
			const char* home_env = std::getenv("HOME");
			const std::string home = home_env ? home_env : "";
			std::vector<std::string> paths;
#if defined(__APPLE__)
			const std::string support = home + "/Library/Application Support";
			paths = {support + "/Google/Chrome/NativeMessagingHosts/com.gdownload.host.json",
					 support + "/Microsoft Edge/NativeMessagingHosts/com.gdownload.host.json",
					 support + "/Mozilla/NativeMessagingHosts/com.gdownload.host.json"};
#else
			const char* xdg = std::getenv("XDG_CONFIG_HOME");
			const std::string config = (xdg && xdg[0] == '/') ? std::string(xdg) : home + "/.config";
			paths = {config + "/google-chrome/NativeMessagingHosts/com.gdownload.host.json",
					 config + "/microsoft-edge/NativeMessagingHosts/com.gdownload.host.json",
					 config + "/chromium/NativeMessagingHosts/com.gdownload.host.json",
					 home + "/.mozilla/native-messaging-hosts/com.gdownload.host.json"};
#endif
			std::error_code ec;
			for (const auto& p : paths) {
				std::filesystem::remove(p, ec);
			}
			return true;
#endif
		}

	}  // namespace ui
}  // namespace gd

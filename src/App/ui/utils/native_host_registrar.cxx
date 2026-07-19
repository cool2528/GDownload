#include "native_host_registrar.h"

#include <filesystem>
#include <fstream>

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
			// GetExecutableDir 返回正斜杠路径
			return gdl::os::GetExecutableDir() + "/gdownload_native_host.exe";
		}

		std::string NativeHostRegistrar::ManifestDir() {
			return gdl::os::GetAppDataDir() + "/gdownload/native-host";
		}

		bool NativeHostRegistrar::EnsureRegistered() {
#ifdef _WIN32
			const std::string host_exe = HostExecutablePath();
			const std::string dir = ManifestDir();
			std::error_code ec;
			std::filesystem::create_directories(dir, ec);

			const std::string chrome_manifest = dir + "/com.gdownload.host.chrome.json";
			const std::string firefox_manifest = dir + "/com.gdownload.host.firefox.json";

			const std::string chrome_json = BuildManifestJson(
				host_exe, "allowed_origins", "chrome-extension://" + std::string(kChromeExtensionId) + "/");
			const std::string firefox_json =
				BuildManifestJson(host_exe, "allowed_extensions", kFirefoxExtensionId);

			WriteTextFile(chrome_manifest, chrome_json);
			WriteTextFile(firefox_manifest, firefox_json);

			const std::wstring chrome_path_w = Utf8ToWide(chrome_manifest);
			const std::wstring firefox_path_w = Utf8ToWide(firefox_manifest);

			bool any = false;
			// Chrome / Edge / 360 等 Chromium 系共用 Chrome manifest 结构（allowed_origins）
			any |= WriteRegistryDefault(HKEY_CURRENT_USER,
										L"Software\\Google\\Chrome\\NativeMessagingHosts\\com.gdownload.host",
										chrome_path_w);
			any |= WriteRegistryDefault(HKEY_CURRENT_USER,
										L"Software\\Microsoft\\Edge\\NativeMessagingHosts\\com.gdownload.host",
										chrome_path_w);
			// Firefox（allowed_extensions）
			any |= WriteRegistryDefault(HKEY_CURRENT_USER,
										L"Software\\Mozilla\\NativeMessagingHosts\\com.gdownload.host",
										firefox_path_w);

			if (any) {
				LOG_INFO("native messaging host registered (host: {})", host_exe);
			} else {
				LOG_ERR("native messaging host registration failed for all browsers");
			}
			return any;
#else
			// mac/linux：写用户目录 JSON（后续补），此处暂不处理
			return false;
#endif
		}

		bool NativeHostRegistrar::Unregister() {
#ifdef _WIN32
			DeleteRegistryKey(HKEY_CURRENT_USER, L"Software\\Google\\Chrome\\NativeMessagingHosts\\com.gdownload.host");
			DeleteRegistryKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Edge\\NativeMessagingHosts\\com.gdownload.host");
			DeleteRegistryKey(HKEY_CURRENT_USER, L"Software\\Mozilla\\NativeMessagingHosts\\com.gdownload.host");
			return true;
#else
			return false;
#endif
		}

	}  // namespace ui
}  // namespace gd

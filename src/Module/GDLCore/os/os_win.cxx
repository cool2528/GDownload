#include "os_win.h"
#if defined(_WIN32) || defined(_WIN64)
#include <shlobj_core.h>
#include <shlwapi.h>
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shlwapi.lib")
#endif
#include <algorithm>
#include "encoding/encoding.h"
namespace gdl {
	namespace os {
		namespace win {
			namespace detail {
#if defined(_WIN32) || defined(_WIN64)
				String GetKnownFolderPath(REFKNOWNFOLDERID folderId) {
					String result;
					PWSTR path = nullptr;
					if (SHGetKnownFolderPath(folderId, 0, nullptr, &path) == S_OK) {
						result = gdl::encoding::WStringToUtf8(path);
						std::ranges::replace(result, '\\', '/');
						CoTaskMemFree(path);
						return result;
					}
					return result;
				}
#endif
			}  // namespace detail
			String GetUserHomeDir() {
#if defined(_WIN32) || defined(_WIN64)
				return detail::GetKnownFolderPath(FOLDERID_Profile);
#else
				return String("");
#endif
			}

			String GetUserDocumentsDir() {
#if defined(_WIN32) || defined(_WIN64)
				return detail::GetKnownFolderPath(FOLDERID_Documents);
#else
				return String("");
#endif
			}

			String GetUserDownloadsDir() {
#if defined(_WIN32) || defined(_WIN64)
				return detail::GetKnownFolderPath(FOLDERID_Downloads);
#else
				return String("");
#endif
			}

			String GetUserDesktopDir() {
#if defined(_WIN32) || defined(_WIN64)
				return detail::GetKnownFolderPath(FOLDERID_Desktop);
#else
				return String("");
#endif
			}

			String GetUserVideosDir() {
#if defined(_WIN32) || defined(_WIN64)
				return detail::GetKnownFolderPath(FOLDERID_Videos);
#else
				return String("");
#endif
			}

			String GetUserMusicDir() {
#if defined(_WIN32) || defined(_WIN64)
				return detail::GetKnownFolderPath(FOLDERID_Music);
#else
				return String("");
#endif
			}

			String GetUserPicturesDir() {
#if defined(_WIN32) || defined(_WIN64)
				return detail::GetKnownFolderPath(FOLDERID_Pictures);
#else
				return String("");
#endif
			}

			String GetAppDataDir() {
#if defined(_WIN32) || defined(_WIN64)
				return detail::GetKnownFolderPath(FOLDERID_RoamingAppData);
#else
				return String("");
#endif
			}

			String GetTempDir() {
#if defined(_WIN32) || defined(_WIN64)
				TCHAR buffer[MAX_PATH];
				::GetTempPath(MAX_PATH, buffer);
				auto temp_path = gdl::encoding::WStringToUtf8(buffer);
				std::ranges::replace(temp_path, '\\', '/');
				return temp_path;
#else
				return String("");
#endif
			}

			String GetExecutableDir() {
#if defined(_WIN32) || defined(_WIN64)
				TCHAR buffer[MAX_PATH];
				::GetModuleFileName(nullptr, buffer, MAX_PATH);
				::PathRemoveFileSpec(buffer);
				auto run_path = gdl::encoding::WStringToUtf8(buffer);
				std::ranges::replace(run_path, '\\', '/');
				return run_path;
#else
				return String("");
#endif
			}

			String GetCurrentWorkingDir() {
#if defined(_WIN32) || defined(_WIN64)
				return GetExecutableDir();
#else
				return String("");
#endif
			}

			std::optional<std::pair<String, int>> GetSystemHTTPProxy() {
#if defined(_WIN32) || defined(_WIN64)
				WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyInfo;

				if (WinHttpGetIEProxyConfigForCurrentUser(&proxyInfo)) {
					if (proxyInfo.lpszProxy) {
						String proxyServer = gdl::encoding::WStringToAnsi(proxyInfo.lpszProxy);
						int proxyPort	   = 0;
						size_t colonPos	   = proxyServer.find(":");
						if (colonPos != std::wstring::npos) {
							proxyPort	= std::stoi(proxyServer.substr(colonPos + 1));
							proxyServer = proxyServer.substr(0, colonPos);
						}
						if (proxyInfo.lpszProxy) {
							GlobalFree(proxyInfo.lpszProxy);
						}
						if (proxyInfo.lpszProxyBypass) {
							GlobalFree(proxyInfo.lpszProxyBypass);
						}
						if (proxyInfo.lpszAutoConfigUrl) {
							GlobalFree(proxyInfo.lpszAutoConfigUrl);
						}
						return std::make_pair(proxyServer, proxyPort);
					}
				}
				return std::nullopt;
#else
				return std::nullopt;
#endif
			}

		}  // namespace win
	}  // namespace os

}  // namespace gdl

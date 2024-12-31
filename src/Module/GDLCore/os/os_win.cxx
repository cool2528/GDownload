#include "os_win.h"
#if defined(_WIN32 ) || defined(_WIN64)
#include <windows.h>
#include <winhttp.h>
#include <shlobj_core.h>
#include <shlwapi.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shlwapi.lib")
#endif
#include "encoding/encoding.h"
#include <algorithm>
namespace gdl {
	namespace os {
		namespace win {
			namespace detail {
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
			}
			String GetUserHomeDir() {
				return detail::GetKnownFolderPath(FOLDERID_Profile);
			}

			String GetUserDocumentsDir() {
				return detail::GetKnownFolderPath(FOLDERID_Documents);
			}

			String GetUserDownloadsDir() {
				return detail::GetKnownFolderPath(FOLDERID_Downloads);
			}

			String GetUserDesktopDir() {
				return detail::GetKnownFolderPath(FOLDERID_Desktop);
			}

			String GetUserVideosDir() {
				return detail::GetKnownFolderPath(FOLDERID_Videos);
			}

			String GetUserMusicDir() {
				return detail::GetKnownFolderPath(FOLDERID_Music);
			}

			String GetUserPicturesDir() {
				return detail::GetKnownFolderPath(FOLDERID_Pictures);
			}

			String GetAppDataDir() {
				return detail::GetKnownFolderPath(FOLDERID_RoamingAppData);
			}

			String GetTempDir() {
				TCHAR buffer[MAX_PATH];
				::GetTempPath(MAX_PATH, buffer);
				auto temp_path = gdl::encoding::WStringToUtf8(buffer);
				std::ranges::replace(temp_path, '\\', '/');
				return temp_path;
			}

			String GetExecutableDir() {
				TCHAR buffer[MAX_PATH];
				::GetModuleFileName(nullptr, buffer, MAX_PATH);
				::PathRemoveFileSpec(buffer);
				auto run_path =  gdl::encoding::WStringToUtf8(buffer);
				std::ranges::replace(run_path, '\\', '/');
				return run_path;
			}

			String GetCurrentWorkingDir() {
				return GetExecutableDir();
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

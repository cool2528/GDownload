#include "os_win.h"
#ifdef _WIN32 || defined(_WIN64)
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif
namespace gdl {
	namespace os {
		namespace win {
			String GetUserHomeDir() {
				return String();
			}

			String GetUserDocumentsDir() {
				return String();
			}

			String GetUserDownloadsDir() {
				return String();
			}

			String GetUserDesktopDir() {
				return String();
			}

			String GetUserVideosDir() {
				return String();
			}

			String GetUserMusicDir() {
				return String();
			}

			String GetUserPicturesDir() {
				return String();
			}

			String GetAppDataDir() {
				return String();
			}

			String GetTempDir() {
				return String();
			}

			String GetExecutableDir() {
				return String();
			}

			String GetCurrentWorkingDir() {
				return String();
			}

			std::optional<std::pair<String, int>> GetSystemHTTPProxy() {
#if defined(_WIN32) || defined(_WIN64)
				WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyInfo;

				if (WinHttpGetIEProxyConfigForCurrentUser(&proxyInfo)) {
					if (proxyInfo.lpszProxy) {
						mbstate_t state = {};
						char buffer[256];
						const wchar_t* src = proxyInfo.lpszProxy;
						size_t len = wcsrtombs(buffer, &src, sizeof(buffer), &state);
						if (len != (size_t)-1) {
							String proxyServer = buffer;
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
				}
				return std::nullopt;
#else
				return std::nullopt;
#endif
			}

		}  // namespace win
	}  // namespace os

}  // namespace gdl

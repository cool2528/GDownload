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
#ifdef _WIN32 || defined(_WIN64)
				WINHTTP_PROXY_INFO proxyInfo;

				if (WinHttpGetIEProxyConfigForCurrentUser(&proxyInfo)) {
					if (proxyInfo.dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY) {
						String proxyServer = proxyInfo.lpszProxy;
						int proxyPort	   = 0;
						size_t colonPos	   = proxyServer.find(":");
						if (colonPos != std::string::npos) {
							proxyPort	= std::stoi(proxyServer.substr(colonPos + 1));
							proxyServer = proxyServer.substr(0, colonPos);
						}
						if (proxyInfo.lpszProxy) {
							GlobalFree(proxyInfo.lpszProxy);
						}
						return std::make_pair(proxyServer, proxyPort);
					}
				}
#else
				return std::nullopt;
#endif
			}

		}  // namespace win
	}  // namespace os

}  // namespace gdl

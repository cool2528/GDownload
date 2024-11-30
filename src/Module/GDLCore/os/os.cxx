#include "os.h"
#ifdef _WIN32
#include "os_win.h"
#elif defined(__APPLE__)
#include "os_mac.h"
#else
#include "os_linux.h"
#endif
namespace gdl {
	namespace os {
		String GetUserHomeDir() {
#ifdef _WIN32
			return win::GetUserHomeDir();
#elif defined(__APPLE__)
			return mac::GetUserHomeDir();
#else
			return linux::GetUserHomeDir();
#endif
		}

		String GetUserDocumentsDir() {
#ifdef _WIN32
			return win::GetUserDocumentsDir();
#elif defined(__APPLE__)
			return mac::GetUserDocumentsDir();
#else
			return linux::GetUserDocumentsDir();
#endif
		}

		String GetUserDownloadsDir() {
#ifdef _WIN32
			return win::GetUserDownloadsDir();
#elif defined(__APPLE__)
			return mac::GetUserDownloadsDir();
#else
			return linux::GetUserDownloadsDir();
#endif
		}

		String GetUserDesktopDir() {
#ifdef _WIN32
			return win::GetUserDesktopDir();
#elif defined(__APPLE__)
			return mac::GetUserDesktopDir();
#else
			return linux::GetUserDesktopDir();
#endif
		}

		String GetUserVideosDir() {
#ifdef _WIN32
			return win::GetUserVideosDir();
#elif defined(__APPLE__)
			return mac::GetUserVideosDir();
#else
			return linux::GetUserVideosDir();
#endif
		}

		String GetUserMusicDir() {
#ifdef _WIN32
			return win::GetUserMusicDir();
#elif defined(__APPLE__)
			return mac::GetUserMusicDir();
#else
			return linux::GetUserMusicDir();
#endif
		}

		String GetUserPicturesDir() {
#ifdef _WIN32
			return win::GetUserPicturesDir();
#elif defined(__APPLE__)
			return mac::GetUserPicturesDir();
#else
			return linux::GetUserPicturesDir();
#endif
		}

		String GetAppDataDir() {
#ifdef _WIN32
			return win::GetAppDataDir();
#elif defined(__APPLE__)
			return mac::GetAppDataDir();
#else
			return linux::GetAppDataDir();
#endif
		}

		String GetTempDir() {
#ifdef _WIN32
			return win::GetTempDir();
#elif defined(__APPLE__)
			return mac::GetTempDir();
#else
			return linux::GetTempDir();
#endif
		}

		String GetExecutableDir() {
#ifdef _WIN32
			return win::GetExecutableDir();
#elif defined(__APPLE__)
			return mac::GetExecutableDir();
#else
			return linux::GetExecutableDir();
#endif
		}

		String GetCurrentWorkingDir() {
#ifdef _WIN32
			return win::GetCurrentWorkingDir();
#elif defined(__APPLE__)
			return mac::GetCurrentWorkingDir();
#else
			return linux::GetCurrentWorkingDir();
#endif
		}

		std::optional<std::pair<String, int>> GetSystemHTTPProxy() {
#ifdef _WIN32
			return win::GetSystemHTTPProxy();
#elif defined(__APPLE__)
			return mac::GetSystemHTTPProxy();
#else
			return linux::GetSystemHTTPProxy();
#endif
		}

	}  // namespace os

}  // namespace gdl

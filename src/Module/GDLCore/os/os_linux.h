#pragma once
#include <optional>

#include "globalTypes.h"
namespace gdl {
	namespace os {
		namespace linux_os {
			String GetUserHomeDir();
			String GetUserDocumentsDir();
			String GetUserDownloadsDir();
			String GetUserDesktopDir();
			String GetUserVideosDir();
			String GetUserMusicDir();
			String GetUserPicturesDir();

			String GetAppDataDir();
			String GetTempDir();
			String GetExecutableDir();
			String GetCurrentWorkingDir();
			std::optional<std::pair<String, int>> GetSystemHTTPProxy();
		}  // namespace linux_os
	}  // namespace os
}  // namespace gdl

#pragma once
#include "globalTypes.h"
#include <string>
#include <optional>
#include <utility>
namespace gdl {
	namespace os {
		namespace mac {
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
		}  // namespace mac
	}  // namespace os
}  // namespace gdl

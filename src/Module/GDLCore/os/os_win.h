#pragma once
#include "globalTypes.h"
namespace gdl {
	namespace osx {
		namespace win {
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
		}  // namespace win
	}  // namespace osx
}  // namespace gdl

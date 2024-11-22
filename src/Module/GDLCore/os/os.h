#pragma once
#include "export.h"
#include "globalTypes.h"
/**
 * @brief 实现一些跨平台通用的系统目录的API
 */
namespace gdl {
	namespace os {

		GDLCore_API String GetUserHomeDir();
		GDLCore_API String GetUserDocumentsDir();
		GDLCore_API String GetUserDownloadsDir();
		GDLCore_API String GetUserDesktopDir();
		GDLCore_API String GetUserVideosDir();
		GDLCore_API String GetUserMusicDir();
		GDLCore_API String GetUserPicturesDir();

		GDLCore_API String GetAppDataDir();
		GDLCore_API String GetTempDir();
		GDLCore_API String GetExecutableDir();
		GDLCore_API String GetCurrentWorkingDir();

	}  // namespace os

}  // namespace gdl

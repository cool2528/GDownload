#include "os_linux.h"
#ifdef __linux__
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <cstdlib>
#include <cstdio>
#include <limits.h>
#include <linux/limits.h>
namespace gdl {
	namespace os {
		namespace linux_os {
			String GetUserHomeDir() {
				const char* home_dir = getenv("HOME");
				if (home_dir) {
					return String(home_dir);
				}
				return String();
			}

			String GetUserDocumentsDir() {
				// 首先检查 XDG_CONFIG_HOME/user-dirs.dirs
				const char* xdg_config = getenv("XDG_CONFIG_HOME");
				String config_dir = xdg_config && *xdg_config == '/' ? 
					String(xdg_config) : GetUserHomeDir() + "/.config";
				
				// 如果找不到配置文件，使用默认路径
				return GetUserHomeDir() + "/Documents";
			}

			String GetUserDownloadsDir() {
				return GetUserHomeDir() + "/Downloads";
			}

			String GetUserDesktopDir() {
				return GetUserHomeDir() + "/Desktop";
			}

			String GetUserVideosDir() {
				return GetUserHomeDir() + "/Videos";
			}

			String GetUserMusicDir() {
				return GetUserHomeDir() + "/Music";
			}

			String GetUserPicturesDir() {
				return GetUserHomeDir() + "/Pictures";
			}

			String GetAppDataDir() {
				const char* xdg_data = getenv("XDG_DATA_HOME");
				if (xdg_data && *xdg_data == '/') {
					return String(xdg_data);
				}
				return GetUserHomeDir() + "/.local/share";
			}

			String GetTempDir() {
				const char* tmp_dir = getenv("TMPDIR");
				if (tmp_dir && *tmp_dir == '/') {
					return String(tmp_dir);
				}
				return String("/tmp");
			}

			String GetExecutableDir() {
				char buf[PATH_MAX];
				ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
				if (len != -1) {
					buf[len] = '\0';
					String path(buf);
					size_t last_slash = path.rfind('/');
					if (last_slash != String::npos) {
						return path.substr(0, last_slash);
					}
				}
				return String();
			}

			String GetCurrentWorkingDir() {
				char buf[PATH_MAX];
				if (getcwd(buf, sizeof(buf))) {
					return String(buf);
				}
				return String();
			}

			std::optional<std::pair<String, int>> GetSystemHTTPProxy() {
				return std::nullopt;
			}

		}  // namespace linux_os
	}  // namespace os

}  // namespace gdl
#endif
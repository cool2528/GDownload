#include "os_linux.h"
#ifdef __linux__
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <cstdlib>
#include <cstdio>
#include <fstream>
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

			// 解析 ~/.config/user-dirs.dirs 中的 XDG_XXX_DIR，取不到返回空串（C4）
			String ParseXdgUserDir(const String& key, const String& home) {
				const char* xdg_config = getenv("XDG_CONFIG_HOME");
				String config_home = (xdg_config && *xdg_config == '/') ? String(xdg_config) : home + "/.config";
				std::ifstream file(config_home + "/user-dirs.dirs");
				if (!file) return String();
				String line;
				while (std::getline(file, line)) {
					if (line.empty() || line[0] == '#') continue;
					size_t eq = line.find('=');
					if (eq == String::npos || line.compare(0, eq, key) != 0) continue;
					String value = line.substr(eq + 1);
					size_t q1 = value.find('"');
					size_t q2 = value.rfind('"');
					if (q1 != String::npos && q2 > q1) value = value.substr(q1 + 1, q2 - q1 - 1);
					const String home_var = "$HOME";
					if (value.rfind(home_var, 0) == 0) value = home + value.substr(home_var.size());
					return value;
				}
			return String();
			}

			String GetUserDocumentsDir() {
				String home = GetUserHomeDir();
				String xdg = ParseXdgUserDir("XDG_DOCUMENTS_DIR", home);
				return xdg.empty() ? home + "/Documents" : xdg;
			}

			String GetUserDownloadsDir() {
				String home = GetUserHomeDir();
				String xdg = ParseXdgUserDir("XDG_DOWNLOAD_DIR", home);
				return xdg.empty() ? home + "/Downloads" : xdg;
			}

			String GetUserDesktopDir() {
				String home = GetUserHomeDir();
				String xdg = ParseXdgUserDir("XDG_DESKTOP_DIR", home);
				return xdg.empty() ? home + "/Desktop" : xdg;
			}

			String GetUserVideosDir() {
				String home = GetUserHomeDir();
				String xdg = ParseXdgUserDir("XDG_VIDEOS_DIR", home);
				return xdg.empty() ? home + "/Videos" : xdg;
			}

			String GetUserMusicDir() {
				String home = GetUserHomeDir();
				String xdg = ParseXdgUserDir("XDG_MUSIC_DIR", home);
				return xdg.empty() ? home + "/Music" : xdg;
			}

			String GetUserPicturesDir() {
				String home = GetUserHomeDir();
				String xdg = ParseXdgUserDir("XDG_PICTURES_DIR", home);
				return xdg.empty() ? home + "/Pictures" : xdg;
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
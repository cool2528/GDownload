#include "install_guide_manager.h"

#include <QFileInfo>
#include <QString>
#include <QVariantMap>

#include "os/os.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace gd {
	namespace ui {

		namespace {
#ifdef _WIN32
			// 检查 App Paths 是否存在指定可执行文件项（HKLM/HKCU，64 位视图）
			bool AppPathExists(const std::wstring& exe_name) {
				const std::wstring subkey =
					L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\" + exe_name;
				for (HKEY root : {HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER}) {
					HKEY hkey = nullptr;
					if (::RegOpenKeyExW(root, subkey.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &hkey) == ERROR_SUCCESS) {
						::RegCloseKey(hkey);
						return true;
					}
				}
				return false;
			}
#endif
		}  // namespace

		InstallGuideManager& InstallGuideManager::Instance() {
			static InstallGuideManager instance;
			return instance;
		}

		InstallGuideManager::InstallGuideManager(QObject* parent) : QObject(parent) {
			refresh();
		}

		QString InstallGuideManager::MarkerPath() {
			return QString::fromStdString(gdl::os::GetAppDataDir()) + "/gdownload/native-host/handshake.marker";
		}

		void InstallGuideManager::refresh() {
			const bool now_paired = QFileInfo::exists(MarkerPath());
			if (now_paired != paired_) {
				paired_ = now_paired;
				emit pairedChanged();
			}
		}

		QVariantList InstallGuideManager::detectedBrowsers() const {
			QVariantList result;
#ifdef _WIN32
			struct Browser {
				const char* id;
				const char* name;
				const wchar_t* exe;
			};
			static const Browser browsers[] = {
				{"chrome", "Google Chrome", L"chrome.exe"},
				{"edge", "Microsoft Edge", L"msedge.exe"},
				{"firefox", "Mozilla Firefox", L"firefox.exe"},
				{"360se", "360 安全浏览器", L"360se.exe"},
				{"360chrome", "360 极速浏览器", L"360chrome.exe"},
				{"qq", "QQ 浏览器", L"QQBrowser.exe"},
				{"sogou", "搜狗浏览器", L"SogouExplorer.exe"},
			};
			for (const auto& b : browsers) {
				if (AppPathExists(b.exe)) {
					QVariantMap item;
					item["id"] = QString::fromLatin1(b.id);
					item["name"] = QString::fromUtf8(b.name);
					item["installed"] = true;
					result.append(item);
				}
			}
#endif
			return result;
		}

	}  // namespace ui
}  // namespace gd

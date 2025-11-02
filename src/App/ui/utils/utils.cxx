#include "utils.h"
#include <qdir.h>
#include <QClipboard>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include "GDLCore/logger.h"
#include "clipboard_watcher.h"
#if defined(_WIN32) || defined(_WIN64)
#include <shobjidl.h>
#include <windows.h>
#endif
#include "version.h"
#include <QString>
#include <QCoreApplication>
#include <QDir>
#include <QProcess>

#ifdef Q_OS_WIN
#include <QSettings>
#elif defined(Q_OS_MACOS)
#include <QProcess>
#include <QStandardPaths>
#elif defined(Q_OS_LINUX)
#include <QFile>
#include <QTextStream>
#endif

namespace gdl {

	namespace ui {
		namespace utils {
			UtilsToolsManager::~UtilsToolsManager() {}

            bool UtilsToolsManager::SetClipboardText(const QString& text) {
                QClipboard* clipboard = QGuiApplication::clipboard();
                clipboard->setText(text);
                return true;
            }
            void UtilsToolsManager::SetTaskbarProgress(double progress, void* nativeWindowHandle) {
#if defined(_WIN32) || defined(_WIN64)
                if (nativeWindowHandle) {
                    ITaskbarList3* pTaskbar = nullptr;
                    if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList3,
                                                   (void**)&pTaskbar))) {
                        HWND hwnd = reinterpret_cast<HWND>(nativeWindowHandle);
                        if (progress >= 1.0) {
                            pTaskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
                        }
                        else {
                            pTaskbar->SetProgressValue(hwnd, static_cast<ULONGLONG>(progress * 100), 100);
                            pTaskbar->SetProgressState(hwnd, TBPF_NORMAL);
                        }

                        pTaskbar->Release();
                    }
                }
#elif defined(__APPLE__)
                setTaskbarProgress(progress);
#else
                Q_UNUSED(progress);
                Q_UNUSED(nativeWindowHandle);
                LOG_ERR("SetTaskbarProgress not implemented for this platform")
#endif
            }

            QString UtilsToolsManager::Version() const {
                return GDownload_VERSION_STRING;
            }

            QString UtilsToolsManager::GetNoticeContent() const {
                QFile file(":/docs/NOTICE");
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    return "";
                }
                QTextStream in(&file);
                QString content = in.readAll();
                file.close();
                return content;
            }

#ifdef __APPLE__
			void UtilsToolsManager::HideMacOsxWindowStandardButtons(QQuickWindow* window) {
				if (window) {
					hideWindowStandardButtons(window->winId());
				}
			}
#endif

			UtilsToolsManager::UtilsToolsManager(QObject* parent) {}

			void RegisterTypes(QQmlEngine* engine) {
				if (!engine) {
					LOG_ERR("invalid QQmlEngine");
					return;
				}
				engine->rootContext()->setContextProperty("UtilsToolsManager", &UtilsToolsManager::Instance());
                engine->rootContext()->setContextProperty("ClipboardWatcher", &ClipboardWatcher::Instance());
			}

            bool UtilsToolsManager::SetAutoStart(bool enable) {
                return SetAutoStartImpl(enable);
            }

            bool UtilsToolsManager::IsAutoStartEnabled() const {
                return IsAutoStartEnabledImpl();
            }

            bool UtilsToolsManager::RelaunchAfterExit(int delayMs) {
                if (delayMs < 0) {
                    delayMs = 0;
                }

                const QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
                const QString workDir = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
                const qint64 pid = QCoreApplication::applicationPid();

#ifdef Q_OS_WIN
                QString exeEscaped = exePath;
                exeEscaped.replace("'", "''");
                QString workDirEscaped = workDir;
                workDirEscaped.replace("'", "''");

                const QString command = QStringLiteral(
                    "$pid=%1; "
                    "while (Get-Process -Id $pid -ErrorAction SilentlyContinue) { Start-Sleep -Milliseconds 100 }; "
                    "Start-Sleep -Milliseconds %2; "
                    "Start-Process -FilePath '%3' -WorkingDirectory '%4'"
                ).arg(pid).arg(delayMs).arg(exeEscaped).arg(workDirEscaped);

                const QStringList args = {
                    QStringLiteral("-NoProfile"),
                    QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"),
                    QStringLiteral("-Command"), command
                };

                if (!QProcess::startDetached(QStringLiteral("powershell.exe"), args, workDir)) {
                    LOG_ERR("RelaunchAfterExit: failed to invoke powershell.exe");
                    return false;
                }
                return true;

#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
                QString exeEscaped = exePath;
                exeEscaped.replace("'", "'\\''");
                QString workDirEscaped = workDir;
                workDirEscaped.replace("'", "'\\''");

                const QString script = QStringLiteral(
                    "pid=%1; "
                    "while kill -0 $pid 2>/dev/null; do sleep 0.1; done; "
                    "sleep %2; "
                    "cd '%4'; "
                    "exec '%3' &"
                ).arg(pid)
                 .arg(QString::number(delayMs / 1000.0, 'f', 3))
                 .arg(exeEscaped)
                 .arg(workDirEscaped);

                if (!QProcess::startDetached(QStringLiteral("/bin/sh"), { QStringLiteral("-c"), script }, workDir)) {
                    LOG_ERR("RelaunchAfterExit: failed to invoke /bin/sh relay script");
                    return false;
                }
                return true;
#else
                Q_UNUSED(delayMs);
                Q_UNUSED(exePath);
                Q_UNUSED(workDir);
                return false;
#endif
            }

            bool UtilsToolsManager::SetAutoStartImpl(bool enable) {
#ifdef Q_OS_WIN
                QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                                  QSettings::NativeFormat);
                QString appName = QCoreApplication::applicationName();
                QString appPath = QCoreApplication::applicationFilePath().replace('/', '\\');
                
                if (enable) {
                    settings.setValue(appName, appPath);
                } else {
                    settings.remove(appName);
                }
                return true;

#elif defined(Q_OS_MACOS)
                QString appName = QCoreApplication::applicationName();
                QString plistPath = QDir::homePath() + 
                                   "/Library/LaunchAgents/com." + appName.toLower() + ".plist";
                
                if (enable) {
                    QString plistContent = QString(
                        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
                        "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
                        "<plist version=\"1.0\">\n"
                        "<dict>\n"
                        "    <key>Label</key>\n"
                        "    <string>com.%1</string>\n"
                        "    <key>ProgramArguments</key>\n"
                        "    <array>\n"
                        "        <string>%2</string>\n"
                        "    </array>\n"
                        "    <key>RunAtLoad</key>\n"
                        "    <true/>\n"
                        "</dict>\n"
                        "</plist>").arg(appName.toLower())
                                  .arg(QCoreApplication::applicationFilePath());
                                  
                    QFile file(plistPath);
                    if (!file.open(QIODevice::WriteOnly)) {
                        return false;
                    }
                    file.write(plistContent.toUtf8());
                    file.close();
                    
                    // 加载配置
                    QProcess::execute("launchctl", QStringList() << "load" << plistPath);
                } else {
                    // 卸载配置
                    QProcess::execute("launchctl", QStringList() << "unload" << plistPath);
                    QFile::remove(plistPath);
                }
                return true;

#elif defined(Q_OS_LINUX)
                QString appName = QCoreApplication::applicationName();
                QString autostartPath = QDir::homePath() + "/.config/autostart/";
                QString desktopFile = autostartPath + appName.toLower() + ".desktop";
                
                if (enable) {
                    QDir dir(autostartPath);
                    if (!dir.exists()) {
                        dir.mkpath(".");
                    }
                    
                    QString desktopContent = QString(
                        "[Desktop Entry]\n"
                        "Type=Application\n"
                        "Version=1.0\n"
                        "Name=%1\n"
                        "Comment=Start %1 when system starts up\n"
                        "Exec=%2\n"
                        "Terminal=false\n"
                        "Categories=Utility;\n").arg(appName)
                                      .arg(QCoreApplication::applicationFilePath());
                                      
                    QFile file(desktopFile);
                    if (!file.open(QIODevice::WriteOnly)) {
                        return false;
                    }
                    file.write(desktopContent.toUtf8());
                    file.close();
                    
                    // 设置可执行权限
                    QFile::setPermissions(desktopFile, QFile::ReadOwner | QFile::WriteOwner | 
                                                     QFile::ExeOwner | QFile::ReadGroup | 
                                                     QFile::ReadOther);
                } else {
                    QFile::remove(desktopFile);
                }
                return true;
#endif

                return false;
            }

            bool UtilsToolsManager::IsAutoStartEnabledImpl() const {
#ifdef Q_OS_WIN
                QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                                  QSettings::NativeFormat);
                return settings.contains(QCoreApplication::applicationName());

#elif defined(Q_OS_MACOS)
                QString appName = QCoreApplication::applicationName();
                QString plistPath = QDir::homePath() + 
                                   "/Library/LaunchAgents/com." + appName.toLower() + ".plist";
                return QFile::exists(plistPath);

#elif defined(Q_OS_LINUX)
                QString appName = QCoreApplication::applicationName();
                QString desktopFile = QDir::homePath() + "/.config/autostart/" + 
                                     appName.toLower() + ".desktop";
                return QFile::exists(desktopFile);
#endif

                return false;
            }
		}  // namespace utils

	}  // namespace ui

}  // namespace gdl

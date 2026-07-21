#include "utils.h"
#include <qdir.h>
#include <QClipboard>
#include <QFileInfo>
#include <QGuiApplication>
#if !defined(GDL_UTILS_DISABLE_QML_INTEGRATION)
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#endif
#include <utility>
#include "GDLCore/logger.h"
#ifndef GDL_UTILS_DISABLE_QML_INTEGRATION
#include "clipboard_watcher.h"
#endif
#if defined(_WIN32) || defined(_WIN64)
#include <shobjidl.h>
#include <windows.h>
#endif
#include "version.h"
#include <QString>
#include <QCoreApplication>
#include <QDir>
#include <QProcess>

namespace {
    gdl::ui::utils::UtilsToolsManager::ProcessLauncher g_processLauncher;

    bool InvokeProcessLauncher(const QString& program, const QStringList& arguments, const QString& workingDir) {
        if (g_processLauncher) {
            return g_processLauncher(program, arguments, workingDir);
        }
        return QProcess::startDetached(program, arguments, workingDir);
    }
}  // namespace

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

            void UtilsToolsManager::SetProcessLauncherForTesting(ProcessLauncher launcher) {
                g_processLauncher = std::move(launcher);
            }

            void UtilsToolsManager::ResetProcessLauncherForTesting() {
                g_processLauncher = {};
            }

            bool UtilsToolsManager::SetClipboardText(const QString& text) {
                QClipboard* clipboard = QGuiApplication::clipboard();
                clipboard->setText(text);
                return true;
            }

            // 打开文件所在目录并选中该文件；实现照抄 BrowserManagerImpl::ExecutePostDownloadAction
            // 的“打开文件”分支（case 1），不改动 browser_manager 原处以避免回归
            void UtilsToolsManager::OpenContainingFolder(const QString& filePath) {
                if (filePath.isEmpty()) {
                    LOG_WARN("OpenContainingFolder: empty filePath");
                    return;
                }
#if defined(_WIN32) || defined(_WIN64)
                QProcess::startDetached("explorer", {"/select,", QDir::toNativeSeparators(filePath)});
#elif defined(__APPLE__)
                QProcess::startDetached("open", {"-R", filePath});
#else
                QFileInfo fileInfo(filePath);
                QProcess::startDetached("xdg-open", {fileInfo.absolutePath()});
#endif
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

#if defined(__APPLE__) && !defined(GDL_UTILS_DISABLE_QML_INTEGRATION)
			void UtilsToolsManager::HideMacOsxWindowStandardButtons(QQuickWindow* window) {
				if (window) {
					hideWindowStandardButtons(window->winId());
				}
			}
#endif

			UtilsToolsManager::UtilsToolsManager(QObject* parent) {}

			void RegisterTypes(QQmlEngine* engine) {
#ifndef GDL_UTILS_DISABLE_QML_INTEGRATION
				if (!engine) {
					LOG_ERR("invalid QQmlEngine");
					return;
				}
				engine->rootContext()->setContextProperty("UtilsToolsManager", &UtilsToolsManager::Instance());
                engine->rootContext()->setContextProperty("ClipboardWatcher", &ClipboardWatcher::Instance());
#else
                Q_UNUSED(engine);
#endif
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
                // 更完善的路径转义
                QString exeEscaped = exePath;
                exeEscaped.replace("'", "''");
                exeEscaped.replace("\"", "\"\"");
                QString workDirEscaped = workDir;
                workDirEscaped.replace("'", "''");
                workDirEscaped.replace("\"", "\"\"");

                const QString command = QStringLiteral(
                    "$ErrorActionPreference = 'Stop'; "
                    "$targetPid=%1; "
                    "Write-Host 'RelaunchAfterExit: Waiting for PID $targetPid to exit...'; "
                    "while (Get-Process -Id $targetPid -ErrorAction SilentlyContinue) { Start-Sleep -Milliseconds 200 }; "
                    "Write-Host 'RelaunchAfterExit: Process exited, waiting %2ms...'; "
                    "Start-Sleep -Milliseconds %2; "
                    "Write-Host 'RelaunchAfterExit: Starting new process...'; "
                    "try { "
                    "  Start-Process -FilePath '%3' -WorkingDirectory '%4' -PassThru | Out-Null; "
                    "  Write-Host 'RelaunchAfterExit: Successfully started'; "
                    "} catch { "
                    "  Write-Host 'RelaunchAfterExit: Failed to start process: $_'; "
                    "  exit 1; "
                    "}"
                ).arg(pid).arg(delayMs).arg(exeEscaped).arg(workDirEscaped);

                // 尝试多种启动方式
                QStringList powerShellArgs = {
                    QStringLiteral("-NoLogo"),
                    QStringLiteral("-NoProfile"),
                    QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),  // 绕过执行策略限制
                    QStringLiteral("-Command"), command
                };

                LOG_INFO("RelaunchAfterExit: invoking powershell.exe with args: {} workDir: {}",
                         powerShellArgs.join(" ").toStdString(),
                         workDir.toStdString());
                // 首先尝试正常窗口启动（便于调试）
                if (!InvokeProcessLauncher(QStringLiteral("powershell.exe"), powerShellArgs, workDir)) {
                    LOG_ERR("RelaunchAfterExit: failed to invoke powershell.exe with normal window");

                    // 尝试隐藏窗口方式
                    powerShellArgs.insert(1, QStringLiteral("-WindowStyle"));
                    powerShellArgs.insert(2, QStringLiteral("Hidden"));

                    if (!InvokeProcessLauncher(QStringLiteral("powershell.exe"), powerShellArgs, workDir)) {
                        LOG_ERR("RelaunchAfterExit: failed to invoke powershell.exe with hidden window");

                        // 最后尝试使用 cmd.exe 作为备选方案
                        QString cmdPath = exePath;
                        cmdPath.replace("/", "\\");
                        QString cmdCommand = QStringLiteral(
                            "ping 127.0.0.1 -n %1 > nul && \"%2\""
                        ).arg(QString::number((delayMs + 999) / 1000)).arg(cmdPath);

                        QStringList cmdArgs = { QStringLiteral("/c"), cmdCommand };
                        if (InvokeProcessLauncher(QStringLiteral("cmd.exe"), cmdArgs, workDir)) {
                            LOG_INFO("RelaunchAfterExit: successfully invoked cmd.exe fallback");
                            return true;
                        } else {
                            LOG_ERR("RelaunchAfterExit: all relaunch methods failed");
                            return false;
                        }
                    } else {
                        LOG_INFO("RelaunchAfterExit: successfully invoked hidden powershell.exe");
                        return true;
                    }
                } else {
                    LOG_INFO("RelaunchAfterExit: successfully invoked normal powershell.exe");
                    return true;
                }

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

                if (!InvokeProcessLauncher(QStringLiteral("/bin/sh"), { QStringLiteral("-c"), script }, workDir)) {
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

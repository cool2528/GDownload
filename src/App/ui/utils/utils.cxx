#include "utils.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include "GDLCore/logger.h"
#if defined(_WIN32) || defined(_WIN64)
#include <shobjidl.h>
#include <windows.h>
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
			}
		}  // namespace utils

	}  // namespace ui

}  // namespace gdl

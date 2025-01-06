#include "utils.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include "GDLCore/logger.h"
namespace gdl {

	namespace ui {
		namespace utils {
			UtilsToolsManager::~UtilsToolsManager() {}

            bool UtilsToolsManager::SetClipboardText(const QString& text) {
                QClipboard* clipboard = QGuiApplication::clipboard();
                clipboard->setText(text);
                return true;
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

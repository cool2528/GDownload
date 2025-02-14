#include "toast_manager.h"
#include "Definitions/appDef.h"
namespace gdl {
    namespace ui {
        namespace toast {

            ToastManager* ToastManager::create(QQmlEngine*, QJSEngine*) {
                return &Instance();
            }

            ToastManager::~ToastManager() {}

            ToastManager::ToastManager(QObject* parent) : QObject(parent) {}

            void ToastManager::ShowSuccess(const QString& message, int duration) {
                ShowMessage(message, 0, duration);	// 0 对应 MessageToast.Success
            }

            void ToastManager::ShowWarning(const QString& message, int duration) {
                ShowMessage(message, 1, duration);	// 1 对应 MessageToast.Warning
            }

            void ToastManager::ShowInfo(const QString& message, int duration) {
                ShowMessage(message, 2, duration);	// 2 对应 MessageToast.Info
            }

            void ToastManager::ShowError(const QString& message, int duration) {
                ShowMessage(message, 3, duration);	// 3 对应 MessageToast.Error
            }

            void ToastManager::ShowMessage(const QString& message, int type, int duration) {
                emit messageRequested(message, type, duration);
            }

            void RegisterTypes(QQmlEngine* engine) {
                qmlRegisterSingletonInstance<ToastManager>(GEXPORT_MODULE_URL, 1, 0, "ToastManager",
                                                           &ToastManager::Instance());
            }

        }  // namespace toast
    }  // namespace ui
}  // namespace gdl

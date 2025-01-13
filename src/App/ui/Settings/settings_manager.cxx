#include "settings_manager.h"
#include <QQmlEngine>
namespace gdl {
    namespace ui {
        namespace settings {

            Settings::~Settings() {}

            Settings* Settings::create(QQmlEngine* qmlengine, QJSEngine* jsengine) {
                Q_UNUSED(qmlengine)
                Q_UNUSED(jsengine);
                return &Settings::Instance();
            }

            bool Settings::Init() {
                return false;
            }

            void Settings::UnInit()
            {

            }
            Settings::Settings(QObject* parent) {}

            void Settings::Save() {}

            void RegisterTypes(QQmlEngine* engine) {}

        }  // namespace settings
    }  // namespace ui
}  // namespace gdl

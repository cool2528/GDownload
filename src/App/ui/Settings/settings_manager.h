#pragma once
#include <QtQml/qqml.h>
#include <QObject>
#include "setting.h"
#include "singleton.hpp"
class QQmlEngine;
class QJSEngine;
namespace gdl {
    namespace ui {
        namespace settings {

            class Settings : public QObject, public Singleton<Settings> {
                SINGLETON_DECLARE(Settings)
                Q_OBJECT
                QML_SINGLETON
               public:
                ~Settings() override;
                static Settings* create(QQmlEngine*, QJSEngine*);
                bool Init();
                void UnInit();

               private:
                explicit Settings(QObject* parent = nullptr);
                void Save();
                template <class SETTING, class VALUE_TYPE>
                void SetValue(const QString& key, const VALUE_TYPE& value) {
                    auto match = settings::Setting::settings_.find(key);
                    if (match == settings::Setting::settings_.end()) {
                        return;
                    }
                    if (auto setting_ptr = dynamic_cast<SETTING*>(match.value()); setting_ptr) {
                        setting_ptr->Put(value);
                        Save();
                    }
                }

                template <class SETTING, class VALUE_TYPE>
                VALUE_TYPE GetValue(const QString& key) const {
                    auto match = settings::Setting::settings_.find(key);
                    if (match == settings::Setting::settings_.end()) {
                        return VALUE_TYPE();
                    }
                    if (auto setting_ptr = dynamic_cast<SETTING*>(match.value()); setting_ptr) {
                        return setting_ptr->Get();
                    }
                }
            };
            void RegisterTypes(QQmlEngine* engine);
        }  // namespace settings
    }  // namespace ui
}  // namespace gdl

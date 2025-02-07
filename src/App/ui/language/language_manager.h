#pragma once

#include <QObject>
#include <QTranslator>
#include "GDLCore/singleton.hpp"
class QQmlEngine;
namespace gdl {
    namespace ui {
        namespace language {

            class LanguageManager : public QObject, public Singleton<LanguageManager> {
                Q_OBJECT
                SINGLETON_DECLARE(LanguageManager)

               public:
                ~LanguageManager() override;

                // 初始化语言管理器
                void Initialize(QQmlEngine* engine);
                // 切换语言
                Q_INVOKABLE bool SwitchLanguage(const QString& locale);

                // 获取当前语言
                Q_INVOKABLE QString GetCurrentLanguage() const;

                // 获取支持的语言列表
                Q_INVOKABLE QStringList GetSupportedLanguages() const;

               private:
                explicit LanguageManager(QObject* parent = nullptr);

                // 加载翻译文件
                bool LoadTranslation(const QString& locale);

                // 移除当前翻译
                void RemoveCurrentTranslation();

               private:
                QTranslator translator_;
                QQmlEngine* engine_{nullptr};
                QString current_language_;
                QString translation_path_;
                const QString translation_prefix_ = "gdownload_";
            };

            // 注册到QML引擎
            void RegisterTypes(QQmlEngine* engine);

        }  // namespace language
    }  // namespace ui
}  // namespace gdl

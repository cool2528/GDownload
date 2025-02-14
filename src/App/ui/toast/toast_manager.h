#pragma once
#include <QObject>
#include <QQmlEngine>
#include "GDLCore/singleton.hpp"

namespace gdl {
    namespace ui {
        namespace toast {

            class ToastManager : public QObject, public Singleton<ToastManager> {
                Q_OBJECT
                SINGLETON_DECLARE(ToastManager)
                QML_SINGLETON
               public:
                static ToastManager* create(QQmlEngine*, QJSEngine*);
                ~ToastManager() override;

                // 显示成功消息
                Q_INVOKABLE void ShowSuccess(const QString& message, int duration = 3000);

                // 显示警告消息
                Q_INVOKABLE void ShowWarning(const QString& message, int duration = 3000);

                // 显示信息消息
                Q_INVOKABLE void ShowInfo(const QString& message, int duration = 3000);

                // 显示错误消息
                Q_INVOKABLE void ShowError(const QString& message, int duration = 3000);

               private:
                explicit ToastManager(QObject* parent = nullptr);

                // 通用显示方法
                void ShowMessage(const QString& message, int type, int duration);

               Q_SIGNALS:
                // 用于与QML通信的信号
                void messageRequested(const QString& message, int type, int duration);
            };

            // 注册到QML引擎
            void RegisterTypes(QQmlEngine* engine);

        }  // namespace toast
    }  // namespace ui
}  // namespace gdl

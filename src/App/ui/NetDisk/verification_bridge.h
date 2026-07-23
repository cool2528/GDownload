#pragma once
#include <QMutex>
#include <QObject>
#include <QWaitCondition>
#include "GDLCore/singleton.hpp"
#include "PluginManager/IDownload_Plugin.h"

namespace gdl {
    namespace ui {
        namespace netdisk {
            // 插件验证输入桥：worker 线程发起请求并阻塞，UI 线程弹窗收集用户输入后唤醒
            // 同一时刻仅支持一个挂起请求（任务队列为串行执行，天然满足）
            class VerificationBridge : public QObject, public Singleton<VerificationBridge> {
                Q_OBJECT
                SINGLETON_DECLARE(VerificationBridge)
               public:
                ~VerificationBridge() override;
                // 供插件回调线程调用：发出 verificationRequested 并阻塞等待 Submit/Cancel/超时
                // 返回后 param.input_result 为用户输入（空串表示取消或超时）
                void Request(INetDiskDownloadPlugin::VerificationCallbackParam& param);
                // UI 线程调用：提交输入（空白输入等价取消）
                Q_INVOKABLE void Submit(const QString& text);
                // UI 线程调用：取消本次验证
                Q_INVOKABLE void Cancel();

               Q_SIGNALS:
                // 请求 UI 弹窗（跨线程队列投递）
                void verificationRequested(const QString& message, const QString& imageBase64);
                // 等待超时，通知 UI 收起弹窗
                void requestAborted();

               private:
                explicit VerificationBridge(QObject* parent = nullptr);

               private:
                QMutex mutex_;
                QWaitCondition condition_;
                bool pending_{false};
                QString input_;
            };
        }  // namespace netdisk
    }  // namespace ui
}  // namespace gdl

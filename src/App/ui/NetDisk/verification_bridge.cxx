#include "verification_bridge.h"
#include <QDeadlineTimer>

namespace gdl {
    namespace ui {
        namespace netdisk {
            namespace {
                // 等待略短于 JS 运行时为验证交互延长的 5 分钟 deadline，保证先于运行时超时收敛
                constexpr qint64 kWaitTimeoutMs = 290000;
            }  // namespace

            VerificationBridge::VerificationBridge(QObject* parent) : QObject(parent) {}

            VerificationBridge::~VerificationBridge() {
                // 析构时唤醒可能仍在等待的 worker，避免退出阶段悬挂
                QMutexLocker locker(&mutex_);
                pending_ = false;
                condition_.wakeAll();
            }

            void VerificationBridge::Request(INetDiskDownloadPlugin::VerificationCallbackParam& param) {
                {
                    QMutexLocker locker(&mutex_);
                    if (pending_) {
                        // 重入保护：已有挂起请求时直接按取消处理
                        param.input_result.clear();
                        return;
                    }
                    pending_ = true;
                    input_.clear();
                }
                // 信号在锁外发出，接收方（QML，主线程）经队列投递
                Q_EMIT verificationRequested(QString::fromStdString(param.message),
                                             QString::fromStdString(param.image_base64));
                bool timed_out = false;
                {
                    QMutexLocker locker(&mutex_);
                    QDeadlineTimer deadline(kWaitTimeoutMs);
                    while (pending_) {
                        if (!condition_.wait(&mutex_, deadline)) {
                            break;
                        }
                    }
                    if (pending_) {
                        // 超时未收到输入：按取消处理
                        pending_ = false;
                        input_.clear();
                        timed_out = true;
                    }
                    param.input_result = input_.toStdString();
                }
                if (timed_out) {
                    Q_EMIT requestAborted();
                }
            }

            void VerificationBridge::Submit(const QString& text) {
                QMutexLocker locker(&mutex_);
                if (!pending_) {
                    return;
                }
                input_	 = text.trimmed();
                pending_ = false;
                condition_.wakeAll();
            }

            void VerificationBridge::Cancel() {
                QMutexLocker locker(&mutex_);
                if (!pending_) {
                    return;
                }
                input_.clear();
                pending_ = false;
                condition_.wakeAll();
            }
        }  // namespace netdisk
    }  // namespace ui
}  // namespace gdl

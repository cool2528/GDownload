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
                // 唤醒等待中的 worker，并等其完全退出 Request 后才允许销毁，避免释放后访问
                QMutexLocker locker(&mutex_);
                pending_ = false;
                condition_.wakeAll();
                while (active_requests_ > 0) {
                    condition_.wait(&mutex_);
                }
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
                    ++active_requests_;
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
                {
                    // 请求全程（含末尾信号发射）结束后才解除计数，保证析构不会在中途完成
                    QMutexLocker locker(&mutex_);
                    --active_requests_;
                    condition_.wakeAll();
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

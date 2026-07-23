#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <atomic>
#include <functional>
#include <thread>
#include "NetDisk/verification_bridge.h"

using gdl::ui::netdisk::VerificationBridge;

namespace {
    // 主线程泵事件循环直到条件满足或超时
    void PumpUntil(const std::function<bool()>& done, int timeout_ms = 5000) {
        QElapsedTimer timer;
        timer.start();
        while (!done() && timer.elapsed() < timeout_ms) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        }
    }
}  // namespace

TEST(VerificationBridgeTest, SubmitFillsInputResult) {
    auto& bridge = VerificationBridge::Instance();
    std::atomic<bool> requested{false};
    QString got_message;
    QObject receiver;
    QObject::connect(
        &bridge, &VerificationBridge::verificationRequested, &receiver,
        [&](const QString& message, const QString&) {
            got_message = message;
            requested	= true;
        },
        Qt::QueuedConnection);

    INetDiskDownloadPlugin::VerificationCallbackParam param;
    param.message = "need code";
    std::thread worker([&param, &bridge]() { bridge.Request(param); });

    PumpUntil([&requested]() { return requested.load(); });
    ASSERT_TRUE(requested.load());
    EXPECT_EQ(got_message, QString("need code"));

    bridge.Submit("abcd");
    worker.join();
    EXPECT_EQ(param.input_result, "abcd");
}

TEST(VerificationBridgeTest, CancelLeavesInputEmpty) {
    auto& bridge = VerificationBridge::Instance();
    std::atomic<bool> requested{false};
    QObject receiver;
    QObject::connect(
        &bridge, &VerificationBridge::verificationRequested, &receiver,
        [&](const QString&, const QString&) { requested = true; }, Qt::QueuedConnection);

    INetDiskDownloadPlugin::VerificationCallbackParam param;
    std::thread worker([&param, &bridge]() { bridge.Request(param); });
    PumpUntil([&requested]() { return requested.load(); });
    ASSERT_TRUE(requested.load());

    bridge.Cancel();
    worker.join();
    EXPECT_TRUE(param.input_result.empty());
}

TEST(VerificationBridgeTest, SubmitWithoutPendingIsIgnored) {
    auto& bridge = VerificationBridge::Instance();
    // 无挂起请求时调用不应崩溃、不应影响后续请求
    bridge.Submit("stale");
    bridge.Cancel();
    SUCCEED();
}

TEST(VerificationBridgeTest, ReentrantRequestIsRejectedAsCancel) {
    auto& bridge = VerificationBridge::Instance();
    std::atomic<bool> requested{false};
    QObject receiver;
    QObject::connect(
        &bridge, &VerificationBridge::verificationRequested, &receiver,
        [&](const QString&, const QString&) { requested = true; }, Qt::QueuedConnection);

    INetDiskDownloadPlugin::VerificationCallbackParam first;
    std::thread worker([&first, &bridge]() { bridge.Request(first); });
    PumpUntil([&requested]() { return requested.load(); });
    ASSERT_TRUE(requested.load());

    // 第一个请求挂起期间发起第二个请求：应立即按取消返回，不影响第一个请求
    INetDiskDownloadPlugin::VerificationCallbackParam second;
    second.input_result = "stale";
    std::thread reentrant([&second, &bridge]() { bridge.Request(second); });
    reentrant.join();
    EXPECT_TRUE(second.input_result.empty());

    bridge.Submit("abcd");
    worker.join();
    EXPECT_EQ(first.input_result, "abcd");
}

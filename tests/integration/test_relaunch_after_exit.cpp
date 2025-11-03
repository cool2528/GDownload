#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "App/ui/utils/utils.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QProcess>
#include <QTimer>
#include <QDir>
#include <QThread>
#include <QDebug>
#include <QList>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>
#include <QSignalSpy>
#include <QStringList>

class RelaunchAfterExitTest : public ::testing::Test {
protected:
    void SetUp() override {
        testMode_ = false;
        testMarkerFile_ = QDir::temp().filePath("gdownload_relaunch_test.marker");
        launchCalls_.clear();
        failLaunchAttempts_ = 0;
        gdl::ui::utils::UtilsToolsManager::SetProcessLauncherForTesting(
            [this](const QString& program, const QStringList& arguments, const QString& workingDir) {
                launchCalls_.append({program, arguments, workingDir});
                if (failLaunchAttempts_ > 0) {
                    --failLaunchAttempts_;
                    return false;
                }
                return true;
            });

        // 检查是否为重启后的测试
        for (int i = 0; i < QCoreApplication::arguments().size(); ++i) {
            if (QCoreApplication::arguments().at(i) == "--relaunch-test") {
                testMode_ = true;
                break;
            }
        }
    }

    void TearDown() override {
        // 清理测试文件
        if (QFile::exists(testMarkerFile_)) {
            QFile::remove(testMarkerFile_);
        }
        gdl::ui::utils::UtilsToolsManager::ResetProcessLauncherForTesting();
        launchCalls_.clear();
        failLaunchAttempts_ = 0;
    }

    // 创建测试标记文件
    void CreateTestMarker() {
        QFile file(testMarkerFile_);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "Relaunch test started at: " << QDateTime::currentDateTime().toString();
            out << "\nOriginal PID: " << QCoreApplication::arguments().value(1, "unknown");
            out << "\nCurrent PID: " << QCoreApplication::applicationPid();
            file.close();
        }
    }

    // 检查测试标记文件是否存在
    bool TestMarkerExists() {
        return QFile::exists(testMarkerFile_);
    }

    // 读取测试标记文件内容
    QString ReadTestMarker() {
        QFile file(testMarkerFile_);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            return in.readAll();
        }
        return QString();
    }

    bool testMode_;
    QString testMarkerFile_;
    struct LaunchCall {
        QString program;
        QStringList arguments;
        QString workingDirectory;
    };
    QList<LaunchCall> launchCalls_;
    int failLaunchAttempts_ = 0;
};

// 测试重启后的行为
TEST_F(RelaunchAfterExitTest, TestRelaunchedProcess) {
    if (testMode_) {
        // 这是重启后的进程
        CreateTestMarker();

        qDebug() << "✓ 成功重启！这是重启后的进程";
        qDebug() << "当前进程 ID:" << QCoreApplication::applicationPid();

        EXPECT_TRUE(true) << "Relaunched process is running";

        // 短暂延迟后退出
        QThread::msleep(1000);
    } else {
        // 这个测试在原始进程中跳过
        GTEST_SKIP() << "This test runs only in relaunched process";
    }
}

// 测试 RelaunchAfterExit 功能
TEST_F(RelaunchAfterExitTest, TestRelaunchAfterExitIntegration) {
    if (testMode_) {
        GTEST_SKIP() << "This test runs only in original process";
    }

    qDebug() << "=== 开始集成测试：RelaunchAfterExit ===";
    qDebug() << "原始进程 ID:" << QCoreApplication::applicationPid();
    qDebug() << "可执行文件路径:" << QCoreApplication::applicationFilePath();

    // 创建 UtilsToolsManager 实例
    auto& manager = gdl::ui::utils::UtilsToolsManager::Instance();

    // 构建重启命令行参数
    QString appPath = QCoreApplication::applicationFilePath();
    QStringList relaunchArgs;
    relaunchArgs << "--relaunch-test" << QString::number(QCoreApplication::applicationPid());

    // 修改 QCoreApplication 的参数以模拟重启场景
    // 注意：这是一个集成测试，实际调用 RelaunchAfterExit
    bool result = manager.RelaunchAfterExit(2000);  // 2秒延迟

    EXPECT_TRUE(result) << "RelaunchAfterExit should return true";
    ASSERT_FALSE(launchCalls_.isEmpty());
#ifdef Q_OS_WIN
    EXPECT_EQ(launchCalls_.front().program, QStringLiteral("powershell.exe"));
#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    EXPECT_EQ(launchCalls_.front().program, QStringLiteral("/bin/sh"));
#endif

    qDebug() << "RelaunchAfterExit 调用完成，结果:" << result;
    qDebug() << "原始进程将在测试结束后退出，2秒后应该重启";

    // 等待重启发生
    QThread::msleep(500);  // 短暂等待，不等到实际重启

    // 注意：实际的重启验证需要通过检查标记文件来完成
    // 这个测试主要是验证 RelaunchAfterExit 是否能成功启动重启命令
}

// 测试快速重启
TEST_F(RelaunchAfterExitTest, TestQuickRelaunch) {
    if (testMode_) {
        GTEST_SKIP() << "This test runs only in original process";
    }

    auto& manager = gdl::ui::utils::UtilsToolsManager::Instance();

    // 测试 0 延迟重启
    bool result = manager.RelaunchAfterExit(0);
    EXPECT_TRUE(result);
    EXPECT_EQ(launchCalls_.size(), 1);

    qDebug() << "Quick relaunch test completed";
    QThread::msleep(100);
}

// 测试不同平台的重启实现
TEST_F(RelaunchAfterExitTest, TestPlatformSpecificRelaunch) {
    if (testMode_) {
        GTEST_SKIP() << "This test runs only in original process";
    }

    auto& manager = gdl::ui::utils::UtilsToolsManager::Instance();

    qDebug() << "Platform: " <<
#ifdef Q_OS_WIN
        "Windows";
#elif defined(Q_OS_MACOS)
        "macOS";
#elif defined(Q_OS_LINUX)
        "Linux";
#else
        "Unknown";
#endif

    // 测试中等延迟重启
    bool result = manager.RelaunchAfterExit(1000);
    EXPECT_TRUE(result);
    EXPECT_EQ(launchCalls_.size(), 1);

    // 验证应用程序路径的有效性
    QString appPath = QCoreApplication::applicationFilePath();
    EXPECT_TRUE(QFile::exists(appPath)) << "Application path should be valid";

    qDebug() << "Application path:" << appPath;
    qDebug() << "Platform-specific relaunch test completed";

    QThread::msleep(100);
}

// 错误恢复测试
TEST_F(RelaunchAfterExitTest, TestErrorRecovery) {
    if (testMode_) {
        GTEST_SKIP() << "This test runs only in original process";
    }

    auto& manager = gdl::ui::utils::UtilsToolsManager::Instance();

    // 测试多次调用 RelaunchAfterExit
    bool result1 = manager.RelaunchAfterExit(100);
    EXPECT_TRUE(result1);
    EXPECT_EQ(launchCalls_.size(), 1);

    bool result2 = manager.RelaunchAfterExit(200);
    EXPECT_TRUE(result2);
    EXPECT_EQ(launchCalls_.size(), 2);

    bool result3 = manager.RelaunchAfterExit(300);
    EXPECT_TRUE(result3);
    EXPECT_EQ(launchCalls_.size(), 3);

    qDebug() << "Multiple relaunch calls completed successfully";
    QThread::msleep(100);
}

// 性能测试
TEST_F(RelaunchAfterExitTest, TestPerformance) {
    if (testMode_) {
        GTEST_SKIP() << "This test runs only in original process";
    }

    auto& manager = gdl::ui::utils::UtilsToolsManager::Instance();

    // 测量调用 RelaunchAfterExit 所需的时间
    auto startTime = QDateTime::currentMSecsSinceEpoch();

    bool result = manager.RelaunchAfterExit(5000);

    auto endTime = QDateTime::currentMSecsSinceEpoch();
    auto duration = endTime - startTime;

    EXPECT_TRUE(result) << "RelaunchAfterExit should succeed";
    EXPECT_LT(duration, 1000) << "RelaunchAfterExit should return quickly (under 1 second)";
    EXPECT_EQ(launchCalls_.size(), 1);

    qDebug() << "RelaunchAfterExit call took" << duration << "ms";
    QThread::msleep(100);
}

#ifdef Q_OS_WIN
TEST_F(RelaunchAfterExitTest, TestFallbackToCmdWhenPowershellFails) {
    if (testMode_) {
        GTEST_SKIP() << "This test runs only in original process";
    }

    auto& manager = gdl::ui::utils::UtilsToolsManager::Instance();
    failLaunchAttempts_ = 2;

    bool result = manager.RelaunchAfterExit(150);
    EXPECT_TRUE(result);
    EXPECT_EQ(launchCalls_.size(), 3);
    EXPECT_EQ(launchCalls_.back().program, QStringLiteral("cmd.exe"));
}
#endif

#include "test_utils_manager.h"
#include "App/ui/utils/utils.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QList>
#include <QRegularExpression>
#include <QStringList>
#include <QStandardPaths>
#include <QThread>
#include <climits>

void UtilsManagerTest::SetUp() {
    // 初始化测试环境
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
    utilsManager_ = &gdl::ui::utils::UtilsToolsManager::Instance();

    // 清理之前的测试进程
    for (qint64 pid : testProcesses_) {
        KillTestProcess(pid);
    }
    testProcesses_.clear();
}

void UtilsManagerTest::TearDown() {
    // 清理测试创建的进程
    for (qint64 pid : testProcesses_) {
        KillTestProcess(pid);
    }
    testProcesses_.clear();

    gdl::ui::utils::UtilsToolsManager::ResetProcessLauncherForTesting();
    launchCalls_.clear();
    failLaunchAttempts_ = 0;
    utilsManager_.reset();
}

bool UtilsManagerTest::IsProcessRunning(qint64 pid) {
#ifdef Q_OS_WIN
    QProcess process;
    process.start("tasklist", QStringList() << "/FI" << QString("PID eq %1").arg(pid));
    process.waitForFinished(3000);
    QString output = process.readAllStandardOutput();
    return output.contains(QString::number(pid));
#else
    QProcess process;
    process.start("ps", QStringList() << "-p" << QString::number(pid));
    process.waitForFinished(3000);
    return process.exitCode() == 0;
#endif
}

qint64 UtilsManagerTest::StartTestProcess(const QString& command) {
    QProcess process;

#ifdef Q_OS_WIN
    process.start("cmd", QStringList() << "/c" << command);
#else
    process.start("/bin/sh", QStringList() << "-c" << command);
#endif

    if (process.waitForStarted(5000)) {
        qint64 pid = process.pid();
        testProcesses_.append(pid);
        process.detach();  // 分离进程，让它在后台运行
        return pid;
    }
    return -1;
}

void UtilsManagerTest::KillTestProcess(qint64 pid) {
#ifdef Q_OS_WIN
    QProcess::execute("taskkill", QStringList() << "/F" << "/PID" << QString::number(pid));
#else
    QProcess::execute("kill", QStringList() << "-9" << QString::number(pid));
#endif
}

// 测试版本获取
TEST_F(UtilsManagerTest, TestVersion) {
    QString version = utilsManager_->Version();
    EXPECT_FALSE(version.isEmpty());
    EXPECT_TRUE(version.contains(QRegularExpression(R"(\d+\.\d+\.\d+)")));
    qDebug() << "Version:" << version;
}

// 测试剪贴板功能
TEST_F(UtilsManagerTest, TestSetClipboardText) {
    QString testText = "Test clipboard content " + QString::number(QDateTime::currentSecsSinceEpoch());

    bool result = utilsManager_->SetClipboardText(testText);
    EXPECT_TRUE(result);

    // 注意：在无头环境中可能无法测试剪贴板读取，这里只测试写入是否成功
    qDebug() << "Set clipboard text:" << testText;
}

// 测试获取通知内容
TEST_F(UtilsManagerTest, TestGetNoticeContent) {
    QString noticeContent = utilsManager_->GetNoticeContent();

    // 如果 NOTICE 文件存在，内容应该不为空
    if (QFile::exists(":/docs/NOTICE")) {
        EXPECT_FALSE(noticeContent.isEmpty());
        qDebug() << "Notice content length:" << noticeContent.length();
    } else {
        qDebug() << "NOTICE file not found, content is empty";
    }
}

// 测试自启动功能
TEST_F(UtilsManagerTest, TestAutoStart) {
    GTEST_SKIP() << "Auto-start interacts with the real system configuration; skipping in unit tests.";
}

// 测试 RelaunchAfterExit 基本功能（不实际重启）
TEST_F(UtilsManagerTest, TestRelaunchAfterExitBasic) {
    // 这是一个基础测试，测试函数是否能正常返回而不崩溃
    // 实际的重启测试放在集成测试中

    bool result = utilsManager_->RelaunchAfterExit(5000);

    // 函数应该返回 true，表示重启命令已成功启动
    EXPECT_TRUE(result);
    ASSERT_FALSE(launchCalls_.isEmpty());
#ifdef Q_OS_WIN
    EXPECT_EQ(launchCalls_.size(), 1);
    EXPECT_EQ(launchCalls_.front().program, QStringLiteral("powershell.exe"));
#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    EXPECT_EQ(launchCalls_.size(), 1);
    EXPECT_EQ(launchCalls_.front().program, QStringLiteral("/bin/sh"));
#endif
    EXPECT_FALSE(launchCalls_.front().workingDirectory.isEmpty());

    qDebug() << "RelaunchAfterExit basic test completed, result:" << result;

    // 等待一小段时间，确保没有立即崩溃
    QThread::msleep(100);
}

// 测试不同延迟时间的 RelaunchAfterExit
TEST_F(UtilsManagerTest, TestRelaunchAfterExitWithDifferentDelays) {
    // 测试 0 延迟
    bool resultZero = utilsManager_->RelaunchAfterExit(0);
    EXPECT_TRUE(resultZero);
    EXPECT_EQ(launchCalls_.size(), 1);

    // 测试负延迟（应该被修正为 0）
    bool resultNegative = utilsManager_->RelaunchAfterExit(-100);
    EXPECT_TRUE(resultNegative);
    EXPECT_EQ(launchCalls_.size(), 2);

    // 测试较长延迟
    bool resultLong = utilsManager_->RelaunchAfterExit(10000);
    EXPECT_TRUE(resultLong);
    EXPECT_EQ(launchCalls_.size(), 3);

    qDebug() << "RelaunchAfterExit delay tests completed";
}

// 测试路径处理
TEST_F(UtilsManagerTest, TestApplicationPathHandling) {
    QString appPath = QCoreApplication::applicationFilePath();
    QString workDir = QCoreApplication::applicationDirPath();

    EXPECT_FALSE(appPath.isEmpty());
    EXPECT_FALSE(workDir.isEmpty());
    EXPECT_TRUE(QFile::exists(appPath));
    EXPECT_TRUE(QDir(workDir).exists());

    qDebug() << "Application path:" << appPath;
    qDebug() << "Working directory:" << workDir;

    // 测试路径转义逻辑（间接测试）
    bool result = utilsManager_->RelaunchAfterExit(1000);
    EXPECT_TRUE(result);
    EXPECT_EQ(launchCalls_.size(), 1);
    EXPECT_EQ(launchCalls_.front().workingDirectory, workDir);
}

// 错误处理测试
TEST_F(UtilsManagerTest, TestErrorHandling) {
    // 这个测试验证函数在极端情况下的行为

    // 测试非常大的延迟值
    bool resultLargeDelay = utilsManager_->RelaunchAfterExit(INT_MAX);
    EXPECT_TRUE(resultLargeDelay);
    EXPECT_EQ(launchCalls_.size(), 1);

    QThread::msleep(100);

    qDebug() << "Error handling tests completed";
}

#ifdef Q_OS_WIN
// 测试多次失败后的回退逻辑
TEST_F(UtilsManagerTest, TestRelaunchAfterExitFallbackToCmd) {
    failLaunchAttempts_ = 2;  // 让前两个启动尝试失败，触发 cmd.exe 回退

    bool result = utilsManager_->RelaunchAfterExit(200);
    EXPECT_TRUE(result);
    EXPECT_EQ(launchCalls_.size(), 3);
    EXPECT_EQ(launchCalls_.back().program, QStringLiteral("cmd.exe"));
}
#endif

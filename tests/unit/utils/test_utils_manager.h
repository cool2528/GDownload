#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QCoreApplication>
#include <QList>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <memory>

// 需要测试的类前向声明
namespace gdl {
    namespace ui {
        namespace utils {
            class UtilsToolsManager;
        }
    }
}

class UtilsManagerTest : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    // 辅助函数
    bool IsProcessRunning(qint64 pid);
    qint64 StartTestProcess(const QString& command);
    void KillTestProcess(qint64 pid);

    struct LaunchCall {
        QString program;
        QStringList arguments;
        QString workingDirectory;
    };

    // 测试用的成员变量
    gdl::ui::utils::UtilsToolsManager* utilsManager_;
    QList<qint64> testProcesses_;  // 记录测试创建的进程，用于清理
    QList<LaunchCall> launchCalls_;
    int failLaunchAttempts_ = 0;
};

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <QTemporaryDir>
#include <gtest/gtest.h>

#include "Module/GDLCore/logger.h"

TEST(CoreLoggerTest, FlushesMessagesToTheConfiguredFile) {
    QTemporaryDir temporaryDirectory;
    ASSERT_TRUE(temporaryDirectory.isValid());

    const auto logPath = std::filesystem::path(temporaryDirectory.path().toStdString()) / "core.log";
    const std::string marker = "gdownload-core-logger-test-marker";

    ASSERT_TRUE(gdl::InitializeLoggers(logPath.string()));
    gdl::SetLoggerLevel(gdl::LogLevel::kInfo);
    gdl::LogInfo("{}", marker);
    ASSERT_TRUE(gdl::ShutdownLoggers());

    std::ifstream logFile(logPath, std::ios::binary);
    ASSERT_TRUE(logFile.is_open());
    const std::string content((std::istreambuf_iterator<char>(logFile)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find(marker), std::string::npos);
}

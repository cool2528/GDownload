#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QApplication>
#include <QDir>
#include <iostream>

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    QApplication app(argc, argv);

    std::cout << "=== GDownload Unit Tests ===" << std::endl;
    std::cout << "Test binary: " << QApplication::applicationFilePath().toStdString() << std::endl;
    std::cout << "Working directory: " << QDir::currentPath().toStdString() << std::endl;
    std::cout << "================================" << std::endl;

    const int result = RUN_ALL_TESTS();

    std::cout << "================================" << std::endl;
    std::cout << "Tests completed with result: " << result << std::endl;

    return result;
}

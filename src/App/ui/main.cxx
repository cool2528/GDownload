#if defined(WIN32)
#include <windows.h>
#endif
#include <QDir>
#include <QStandardPaths>
#include "GDLCore/logger.h"
#include "view/mainwindow.h"

int main(int argc, char* argv[]) {
	gdl::InitializeLoggers(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString() +
						   "/gdownload/logs/gdownload.log");
	LOG_INFO("init log succeed");
// visual studio clion xcode ...
#if (defined(DEBUG) || defined(_DEBUG))
	gdl::SetLoggerLevel(gdl::LogLevel::kDebug);
#endif
// qt creator ide
#if !defined(QT_NO_DEBUG)
	gdl::SetLoggerLevel(gdl::LogLevel::kDebug);
#endif
	int ret = 0;
	gd::ui::MainWindow mainwindow;
	ret = mainwindow.Exec(argc, argv);
	gdl::ShutdownLoggers();
	return ret;
}

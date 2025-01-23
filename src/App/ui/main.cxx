#if defined(WIN32)
#include <windows.h>
#endif
#include <QDir>
#include "GDLCore/logger.h"
#include "os/os.h"
#include "view/mainwindow.h"
int main(int argc, char* argv[]) {
#if (defined(_WIN32) || defined(_WIN64)) && (defined(DEBUG) || defined(_DEBUG) || !defined(QT_NO_DEBUG))
	::AllocConsole();
#endif
	gdl::InitializeLoggers(gdl::os::GetAppDataDir() + "/gdownload/logs/gdownload.log");
	LOG_INFO("init log succeed");
// visual studio clion xcode ...
#if (defined(DEBUG) || defined(_DEBUG) || !defined(QT_NO_DEBUG))
	gdl::SetLoggerLevel(gdl::LogLevel::kDebug);
#endif

	int ret = 0;
	gd::ui::MainWindow mainwindow;
	ret = mainwindow.Exec(argc, argv);
	gdl::ShutdownLoggers();
#if (defined(_WIN32) || defined(_WIN64)) && (defined(DEBUG) || defined(_DEBUG) || !defined(QT_NO_DEBUG))
	::FreeConsole();
#endif
	return ret;
}

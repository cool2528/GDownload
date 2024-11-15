#if defined(WIN32)
#include <windows.h>
#endif
#include <QDir>
#include <QStandardPaths>
#include "GDLCore/logger.h"
#include "view/mainwindow.h"

int main(int argc, char* argv[]) {
	gdl::InitializeLoggers(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation).toStdString() +
						   "/GDownload/logs/gdownload.log");
	LOG_INFO("init log succeed");
	int ret = 0;
	gd::ui::MainWindow mainwindow;
	ret = mainwindow.Exec(argc, argv);
	return ret;
}

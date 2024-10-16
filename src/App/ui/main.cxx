#if defined(WIN32)
#include <windows.h>
#endif
#include "view/mainwindow.h"
int main(int argc, char *argv[])
{
    
    int ret = 0;
    gd::ui::MainWindow mainwindow;
    ret = mainwindow.Exec(argc,argv);
    return ret;
}

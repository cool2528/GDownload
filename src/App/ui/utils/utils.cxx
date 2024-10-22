#include "utils.h"
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include "GDLCore/logger.h"
namespace gdl {

namespace ui {
namespace utils {
#ifdef __APPLE__
UtilsToolsManager::~UtilsToolsManager()
{

}

void UtilsToolsManager::HideMacOsxWindowStandardButtons(QQuickWindow* window)
{
    if(window){
        hideWindowStandardButtons(window->winId());
    }
}
#endif

UtilsToolsManager::UtilsToolsManager(QObject *parent)
{

}

void RegisterTypes(QQmlEngine *engine)
{
    if(!engine){
        LOG_ERR("invalid QQmlEngine");
        return;
    }
     engine->rootContext()->setContextProperty("UtilsToolsManager",&UtilsToolsManager::Instance());
}
} // utils

} // ui

} // gdl

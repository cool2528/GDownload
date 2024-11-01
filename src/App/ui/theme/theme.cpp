#include "theme.h"
#include <QQmlEngine>
#include "Definitions/appDef.h"
namespace  gdl {

namespace ui {

GTheme *GTheme::create(QQmlEngine * qmlengine, QJSEngine *jsengine)
{
    Q_UNUSED(qmlengine)
    Q_UNUSED(jsengine);
    return &GTheme::Instance();
}

GTheme::GTheme(QObject *parent):QObject(parent)
{

}

void RegisterTypes(QQmlEngine *engine)
{
    qmlRegisterSingletonInstance<GTheme>(GEXPORT_MODULE_URL,1,0,"GTheme",&GTheme::Instance());
}

}
} // gdl

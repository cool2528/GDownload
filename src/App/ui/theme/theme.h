#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include "GDLCore/singleton.hpp"
#include "Definitions/autoProperty.h"
class QQmlEngine;
class QJSEngine;
namespace gdl {

namespace ui {
Q_NAMESPACE
class GTheme :public QObject,public Singleton<GTheme>{
    Q_OBJECT
    QML_AUTO_PROPERTY_V(bool,dark,false)

    QML_NAMED_ELEMENT(GTheme)
    QML_SINGLETON
    SINGLETON_DECLARE(GTheme)
public:
    static GTheme* create(QQmlEngine*,QJSEngine*);
private:
    explicit GTheme(QObject* parent = nullptr);

};

void RegisterTypes(QQmlEngine *engine);

} // ui

} // gdl

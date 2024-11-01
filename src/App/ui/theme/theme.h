#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QColor>
#include "GDLCore/singleton.hpp"
#include "Definitions/autoProperty.h"
#include "Definitions/appDef.h"
class QQmlEngine;
class QJSEngine;
namespace gdl {

namespace ui {
namespace theme {
Q_NAMESPACE
class GTheme :public QObject,public Singleton<GTheme>{
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark  NOTIFY darkChanged)
    QML_AUTO_PROPERTY(GThemeType::ThemeMode,theme)
    QML_AUTO_PROPERTY(QColor,backgroundColor)

    QML_NAMED_ELEMENT(GTheme)
    QML_SINGLETON
    SINGLETON_DECLARE(GTheme)
public:
    static GTheme* create(QQmlEngine*,QJSEngine*);
public:
    bool dark() const;
    Q_SIGNAL void darkChanged();
private:
    explicit GTheme(QObject* parent = nullptr);
    bool SystemIsDarkTheme() const;
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    bool system_is_dark_theme_;

};

void RegisterTypes(QQmlEngine *engine);
} // theme

} // ui

} // gdl

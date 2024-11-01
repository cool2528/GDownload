#pragma once
#include <QObject>
#include <QtQml/qqml.h>
#define GEXPORT_MODULE_URL "gdl.sdk"

// theme type
namespace GThemeType {
Q_NAMESPACE
enum class ThemeMode{
    kSystem = 0x000,
    kLight = 0x001,
    kDark = 0x002
};
Q_ENUM_NS(ThemeMode)
QML_NAMED_ELEMENT(GThemeType)
} //GThemeType


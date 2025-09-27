#include "elementPlusColors.h"
#include "appDef.h"
#include <QQmlEngine>

namespace ElementPlusColors {

ElementPlusColorProvider* ElementPlusColorProvider::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine) {
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    return new ElementPlusColorProvider();
}

ElementPlusColorProvider::ElementPlusColorProvider(QObject* parent) : QObject(parent) {
}

QColor ElementPlusColorProvider::primaryLight(int level) const {
    switch (level) {
        case 1: return QColor(Primary::kPrimaryLight1);
        case 2: return QColor(Primary::kPrimaryLight2);
        case 3: return QColor(Primary::kPrimaryLight3);
        case 4: return QColor(Primary::kPrimaryLight4);
        case 5: return QColor(Primary::kPrimaryLight5);
        case 6: return QColor(Primary::kPrimaryLight6);
        case 7: return QColor(Primary::kPrimaryLight7);
        case 8: return QColor(Primary::kPrimaryLight8);
        case 9: return QColor(Primary::kPrimaryLight9);
        default: return QColor(Primary::kPrimary);
    }
}

QColor ElementPlusColorProvider::textPrimary(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kTextPrimary) : QColor(LightNeutral::kTextPrimary);
}

QColor ElementPlusColorProvider::textRegular(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kTextRegular) : QColor(LightNeutral::kTextRegular);
}

QColor ElementPlusColorProvider::textSecondary(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kTextSecondary) : QColor(LightNeutral::kTextSecondary);
}

QColor ElementPlusColorProvider::textPlaceholder(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kTextPlaceholder) : QColor(LightNeutral::kTextPlaceholder);
}

QColor ElementPlusColorProvider::textDisabled(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kTextDisabled) : QColor(LightNeutral::kTextDisabled);
}

QColor ElementPlusColorProvider::borderBase(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kBorderBase) : QColor(LightNeutral::kBorderBase);
}

QColor ElementPlusColorProvider::borderLight(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kBorderLight) : QColor(LightNeutral::kBorderLight);
}

QColor ElementPlusColorProvider::borderLighter(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kBorderLighter) : QColor(LightNeutral::kBorderLighter);
}

QColor ElementPlusColorProvider::fillBase(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kFillBase) : QColor(LightNeutral::kFillBase);
}

QColor ElementPlusColorProvider::fillLight(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kFillLight) : QColor(LightNeutral::kFillLight);
}

QColor ElementPlusColorProvider::fillLighter(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kFillLighter) : QColor(LightNeutral::kFillLighter);
}

QColor ElementPlusColorProvider::bgWhite(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kBgWhite) : QColor(LightNeutral::kBgWhite);
}

QColor ElementPlusColorProvider::bgPage(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kBgPage) : QColor(LightNeutral::kBgPage);
}

QColor ElementPlusColorProvider::bgBase(bool isDark) const {
    return isDark ? QColor(DarkNeutral::kBgBase) : QColor(LightNeutral::kBgBase);
}

void RegisterElementPlusColors(QQmlEngine* engine) {
    qmlRegisterSingletonType<ElementPlusColorProvider>(
        GEXPORT_MODULE_URL, 1, 0, "ElementPlusColors",
        ElementPlusColorProvider::create
    );
}

} // namespace ElementPlusColors
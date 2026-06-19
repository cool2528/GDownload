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
        case 3: return QColor(Primary::kPrimaryLight3);
        case 5: return QColor(Primary::kPrimaryLight5);
        case 7: return QColor(Primary::kPrimaryLight7);
        case 8: return QColor(Primary::kPrimaryLight8);
        case 9: return QColor(Primary::kPrimaryLight9);
        default: return QColor(Primary::kPrimary);
    }
}

QColor ElementPlusColorProvider::textPrimary(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kTextPrimary) : QColor(AntDesignLight::kTextPrimary);
}

QColor ElementPlusColorProvider::textRegular(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kTextRegular) : QColor(AntDesignLight::kTextRegular);
}

QColor ElementPlusColorProvider::textSecondary(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kTextSecondary) : QColor(AntDesignLight::kTextSecondary);
}

QColor ElementPlusColorProvider::textPlaceholder(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kTextPlaceholder) : QColor(AntDesignLight::kTextPlaceholder);
}

QColor ElementPlusColorProvider::textDisabled(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kTextDisabled) : QColor(AntDesignLight::kTextDisabled);
}

QColor ElementPlusColorProvider::borderBase(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kBorderBase) : QColor(AntDesignLight::kBorderBase);
}

QColor ElementPlusColorProvider::borderLight(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kBorderLight) : QColor(AntDesignLight::kBorderLight);
}

QColor ElementPlusColorProvider::borderLighter(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kBorderLighter) : QColor(AntDesignLight::kBorderLighter);
}

QColor ElementPlusColorProvider::fillBase(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kFillBase) : QColor(AntDesignLight::kFillBase);
}

QColor ElementPlusColorProvider::fillLight(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kFillLight) : QColor(AntDesignLight::kFillLight);
}

QColor ElementPlusColorProvider::fillLighter(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kFillLighter) : QColor(AntDesignLight::kFillLighter);
}

QColor ElementPlusColorProvider::bgWhite(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kBgWhite) : QColor(AntDesignLight::kBgWhite);
}

QColor ElementPlusColorProvider::bgPage(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kBgPage) : QColor(AntDesignLight::kBgPage);
}

QColor ElementPlusColorProvider::bgBase(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kBgBase) : QColor(AntDesignLight::kBgBase);
}

QColor ElementPlusColorProvider::bgOverlay(bool isDark) const {
    return isDark ? QColor(VSCodeDark::kBgOverlay) : QColor(AntDesignLight::kBgOverlay);
}

QColor ElementPlusColorProvider::bgElevated(bool isDark) const {
    // Ant Design 浅色：#FFFFFF 纯白高亮
    // VS Code 暗色：#2D2D30 活动项背景
    return isDark ? QColor(VSCodeDark::kBgElevated) : QColor(AntDesignLight::kBgWhite);
}

void RegisterElementPlusColors(QQmlEngine* engine) {
    qmlRegisterSingletonType<ElementPlusColorProvider>(
        GEXPORT_MODULE_URL, 1, 0, "ElementPlusColors",
        ElementPlusColorProvider::create
    );
}

} // namespace ElementPlusColors
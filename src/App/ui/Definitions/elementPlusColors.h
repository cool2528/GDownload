#pragma once
#include <QObject>
#include <QColor>
#include <QtQml/qqml.h>

namespace ElementPlusColors {
Q_NAMESPACE

// Element Plus 主色系
namespace Primary {
    constexpr const char* kPrimary = "#409EFF";
    constexpr const char* kPrimaryLight1 = "#53A8FF";
    constexpr const char* kPrimaryLight2 = "#66B1FF";
    constexpr const char* kPrimaryLight3 = "#79BBFF";
    constexpr const char* kPrimaryLight4 = "#8CC5FF";
    constexpr const char* kPrimaryLight5 = "#A0CFFF";
    constexpr const char* kPrimaryLight6 = "#B3D8FF";
    constexpr const char* kPrimaryLight7 = "#C6E2FF";
    constexpr const char* kPrimaryLight8 = "#D9ECFF";
    constexpr const char* kPrimaryLight9 = "#ECF5FF";
    constexpr const char* kPrimaryDark2 = "#337ECC";
}

// Element Plus 状态色
namespace Status {
    // 成功色
    constexpr const char* kSuccess = "#67C23A";
    constexpr const char* kSuccessLight3 = "#85CE61";
    constexpr const char* kSuccessLight5 = "#B3E19D";
    constexpr const char* kSuccessLight7 = "#D1EDC4";
    constexpr const char* kSuccessLight8 = "#E1F3D8";
    constexpr const char* kSuccessLight9 = "#F0F9FF";
    constexpr const char* kSuccessDark2 = "#529B2E";

    // 警告色
    constexpr const char* kWarning = "#E6A23C";
    constexpr const char* kWarningLight3 = "#EBB563";
    constexpr const char* kWarningLight5 = "#F3D19E";
    constexpr const char* kWarningLight7 = "#F8E3C5";
    constexpr const char* kWarningLight8 = "#FAECD8";
    constexpr const char* kWarningLight9 = "#FDF6EC";
    constexpr const char* kWarningDark2 = "#B88230";

    // 危险色
    constexpr const char* kDanger = "#F56C6C";
    constexpr const char* kError = "#F56C6C";
    constexpr const char* kDangerLight3 = "#F78989";
    constexpr const char* kDangerLight5 = "#FAB6B6";
    constexpr const char* kDangerLight7 = "#FCD3D3";
    constexpr const char* kDangerLight8 = "#FDE2E2";
    constexpr const char* kDangerLight9 = "#FEF0F0";
    constexpr const char* kDangerDark2 = "#C45656";

    // 信息色
    constexpr const char* kInfo = "#909399";
    constexpr const char* kInfoLight3 = "#A6A9AD";
    constexpr const char* kInfoLight5 = "#C8C9CC";
    constexpr const char* kInfoLight7 = "#DEDFE0";
    constexpr const char* kInfoLight8 = "#E9E9EB";
    constexpr const char* kInfoLight9 = "#F4F4F5";
    constexpr const char* kInfoDark2 = "#73767A";
}

// Element Plus 中性色 - 浅色模式
namespace LightNeutral {
    // 文字色
    constexpr const char* kTextPrimary = "#303133";
    constexpr const char* kTextRegular = "#606266";
    constexpr const char* kTextSecondary = "#909399";
    constexpr const char* kTextPlaceholder = "#A8ABB2";
    constexpr const char* kTextDisabled = "#C0C4CC";

    // 边框色
    constexpr const char* kBorderDarker = "#CDD0D6";
    constexpr const char* kBorderDark = "#D4D7DE";
    constexpr const char* kBorderBase = "#DCDFE6";
    constexpr const char* kBorderLight = "#E4E7ED";
    constexpr const char* kBorderLighter = "#EBEEF5";
    constexpr const char* kBorderExtraLight = "#F2F6FC";

    // 填充色
    constexpr const char* kFillDarker = "#E6E8EB";
    constexpr const char* kFillDark = "#EBEDF0";
    constexpr const char* kFillBase = "#F0F2F5";
    constexpr const char* kFillLight = "#F5F7FA";
    constexpr const char* kFillLighter = "#FAFAFA";
    constexpr const char* kFillBlank = "#FFFFFF";

    // 背景色
    constexpr const char* kBgWhite = "#FFFFFF";
    constexpr const char* kBgPage = "#F2F3F5";
    constexpr const char* kBgBase = "#F5F7FA";
    constexpr const char* kBgOverlay = "#FFFFFF";
}

// Element Plus 中性色 - 暗色模式
namespace DarkNeutral {
    // 文字色
    constexpr const char* kTextPrimary = "#E5EAF3";
    constexpr const char* kTextRegular = "#CFD3DC";
    constexpr const char* kTextSecondary = "#A3A6AD";
    constexpr const char* kTextPlaceholder = "#8D9095";
    constexpr const char* kTextDisabled = "#6C6E72";

    // 边框色
    constexpr const char* kBorderDarker = "#636466";
    constexpr const char* kBorderDark = "#58585B";
    constexpr const char* kBorderBase = "#4C4D4F";
    constexpr const char* kBorderLight = "#414243";
    constexpr const char* kBorderLighter = "#363637";
    constexpr const char* kBorderExtraLight = "#2B2B2C";

    // 填充色
    constexpr const char* kFillDarker = "#424243";
    constexpr const char* kFillDark = "#39393A";
    constexpr const char* kFillBase = "#303030";
    constexpr const char* kFillLight = "#262627";
    constexpr const char* kFillLighter = "#1D1D1D";
    constexpr const char* kFillBlank = "#141414";

    // 背景色
    constexpr const char* kBgWhite = "#141414";
    constexpr const char* kBgPage = "#0A0A0A";
    constexpr const char* kBgBase = "#1D1E1F";
    constexpr const char* kBgOverlay = "#1D1D1D";
}

// Element Plus 尺寸规范
namespace Sizes {
    // 组件尺寸
    constexpr int kLarge = 40;
    constexpr int kDefault = 32;
    constexpr int kSmall = 24;

    // 圆角
    constexpr int kRadiusBase = 4;
    constexpr int kRadiusSmall = 2;
    constexpr int kRadiusRound = 20;
    constexpr int kRadiusCircle = 100;

    // 间距
    constexpr int kSpaceXS = 4;
    constexpr int kSpaceSM = 8;
    constexpr int kSpaceMD = 12;
    constexpr int kSpaceLG = 16;
    constexpr int kSpaceXL = 20;
    constexpr int kSpace2XL = 24;
    constexpr int kSpace3XL = 32;
}

// 工具函数：根据主题模式获取颜色
class ElementPlusColorProvider : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(ElementPlusColors)
    QML_SINGLETON

public:
    static ElementPlusColorProvider* create(QQmlEngine*, QJSEngine*);

    // 主色系
    Q_INVOKABLE QColor primary() const { return QColor(Primary::kPrimary); }
    Q_INVOKABLE QColor primaryLight(int level = 1) const;
    Q_INVOKABLE QColor primaryDark() const { return QColor(Primary::kPrimaryDark2); }

    // 状态色
    Q_INVOKABLE QColor success() const { return QColor(Status::kSuccess); }
    Q_INVOKABLE QColor warning() const { return QColor(Status::kWarning); }
    Q_INVOKABLE QColor danger() const { return QColor(Status::kDanger); }
    Q_INVOKABLE QColor info() const { return QColor(Status::kInfo); }

    // 中性色（根据当前主题模式）
    Q_INVOKABLE QColor textPrimary(bool isDark = false) const;
    Q_INVOKABLE QColor textRegular(bool isDark = false) const;
    Q_INVOKABLE QColor textSecondary(bool isDark = false) const;
    Q_INVOKABLE QColor textPlaceholder(bool isDark = false) const;
    Q_INVOKABLE QColor textDisabled(bool isDark = false) const;

    Q_INVOKABLE QColor borderBase(bool isDark = false) const;
    Q_INVOKABLE QColor borderLight(bool isDark = false) const;
    Q_INVOKABLE QColor borderLighter(bool isDark = false) const;

    Q_INVOKABLE QColor fillBase(bool isDark = false) const;
    Q_INVOKABLE QColor fillLight(bool isDark = false) const;
    Q_INVOKABLE QColor fillLighter(bool isDark = false) const;

    Q_INVOKABLE QColor bgWhite(bool isDark = false) const;
    Q_INVOKABLE QColor bgPage(bool isDark = false) const;
    Q_INVOKABLE QColor bgBase(bool isDark = false) const;

    // 尺寸
    Q_INVOKABLE int sizeLarge() const { return Sizes::kLarge; }
    Q_INVOKABLE int sizeDefault() const { return Sizes::kDefault; }
    Q_INVOKABLE int sizeSmall() const { return Sizes::kSmall; }

    Q_INVOKABLE int radiusBase() const { return Sizes::kRadiusBase; }
    Q_INVOKABLE int radiusSmall() const { return Sizes::kRadiusSmall; }
    Q_INVOKABLE int radiusRound() const { return Sizes::kRadiusRound; }

    Q_INVOKABLE int spaceLG() const { return Sizes::kSpaceLG; }
    Q_INVOKABLE int spaceMD() const { return Sizes::kSpaceMD; }
    Q_INVOKABLE int spaceSM() const { return Sizes::kSpaceSM; }

private:
    explicit ElementPlusColorProvider(QObject* parent = nullptr);
};

QML_DECLARE_TYPE(ElementPlusColorProvider)

void RegisterElementPlusColors(QQmlEngine* engine);

} // namespace ElementPlusColors
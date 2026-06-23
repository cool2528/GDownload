#pragma once
#include <QObject>
#include <QColor>
#include <QtQml/qqml.h>

namespace ElementPlusColors {
Q_NAMESPACE

// Element Plus 主色系 - 基础蓝(#409EFF),色阶严格对齐 EP 官方 light3/5/7/8/9 + dark2
namespace Primary {
    constexpr const char* kPrimary = "#409EFF";        // Element Plus 主色
    constexpr const char* kPrimaryLight3 = "#79BBFF";  // EP light3(hover)
    constexpr const char* kPrimaryLight5 = "#A0CFFF";  // EP light5
    constexpr const char* kPrimaryLight7 = "#C6E2FF";  // EP light7(浅背景)
    constexpr const char* kPrimaryLight8 = "#D9ECFF";  // EP light8(更浅背景)
    constexpr const char* kPrimaryLight9 = "#ECF5FF";  // EP light9(最浅背景)
    constexpr const char* kPrimaryDark2 = "#337ECC";   // EP dark2(active)
}

// Element Plus 状态色
namespace Status {
    // 成功色 - Element Plus Success #67C23A
    constexpr const char* kSuccess = "#67C23A";
    constexpr const char* kSuccessLight3 = "#95D475";
    constexpr const char* kSuccessLight5 = "#B3E19D";
    constexpr const char* kSuccessLight7 = "#D1EDC4";
    constexpr const char* kSuccessLight8 = "#E1F3D8";
    constexpr const char* kSuccessLight9 = "#F0F9EB";
    constexpr const char* kSuccessDark2 = "#529B2E";

    // 警告色 - Element Plus Warning #E6A23C
    constexpr const char* kWarning = "#E6A23C";
    constexpr const char* kWarningLight3 = "#EEBE77";
    constexpr const char* kWarningLight5 = "#F3D19E";
    constexpr const char* kWarningLight7 = "#F8D3A0";
    constexpr const char* kWarningLight8 = "#FAECD8";
    constexpr const char* kWarningLight9 = "#FDF6EC";
    constexpr const char* kWarningDark2 = "#B88230";

    // 危险色 - Element Plus Danger #F56C6C
    constexpr const char* kDanger = "#F56C6C";
    constexpr const char* kError = "#F56C6C";
    constexpr const char* kDangerLight3 = "#F89898";
    constexpr const char* kDangerLight5 = "#FAB6B6";
    constexpr const char* kDangerLight7 = "#FBC4C4";
    constexpr const char* kDangerLight8 = "#FDE2E2";
    constexpr const char* kDangerLight9 = "#FEF0F0";
    constexpr const char* kDangerDark2 = "#C45656";

    // 信息色 - Element Plus Info #909399(灰,不再复用主蓝)
    constexpr const char* kInfo = "#909399";
    constexpr const char* kInfoLight3 = "#B1B3B8";
    constexpr const char* kInfoLight5 = "#C8C9CC";
    constexpr const char* kInfoLight7 = "#DEDEDF";
    constexpr const char* kInfoLight8 = "#E9E9EB";
    constexpr const char* kInfoLight9 = "#F4F4F5";
    constexpr const char* kInfoDark2 = "#73767A";
}

// Element Plus 中性色 - 浅色模式
// 命名空间沿用历史名 AntDesignLight,实际值为 Element Plus 浅色色板,勿再增删 Ant Design 色值
namespace AntDesignLight {
    // 文字色 - Element Plus 浅色(次级/占位色微调冷调,贴合设计稿)
    constexpr const char* kTextPrimary = "#303133";
    constexpr const char* kTextRegular = "#606266";
    constexpr const char* kTextSecondary = "#7B8798";
    constexpr const char* kTextPlaceholder = "#97A3B6";
    constexpr const char* kTextDisabled = "#C0C4CC";

    // 边框色 - 冷调蓝灰(对齐设计稿 #E8EEF7 卡片边框系)
    constexpr const char* kBorderDarker = "#B4C0D4";
    constexpr const char* kBorderDark = "#CAD8EC";
    constexpr const char* kBorderBase = "#D5DEEA";
    constexpr const char* kBorderLight = "#E8EEF7";
    constexpr const char* kBorderLighter = "#EEF3FA";
    constexpr const char* kBorderExtraLight = "#F4F8FD";

    // 填充色 - 冷调蓝灰(输入框/内嵌区)
    constexpr const char* kFillDarker = "#E2E8F2";
    constexpr const char* kFillDark = "#E8EEF7";
    constexpr const char* kFillBase = "#EDF2FA";
    constexpr const char* kFillLight = "#F2F6FC";
    constexpr const char* kFillLighter = "#F7FAFE";
    constexpr const char* kFillBlank = "#FFFFFF";

    // 背景色 - 冷调蓝灰页面底 + 纯白卡片(设计稿 App Shell)
    constexpr const char* kBgWhite = "#FFFFFF";
    constexpr const char* kBgPage = "#EEF2F7";
    constexpr const char* kBgBase = "#F7FAFE";
    constexpr const char* kBgOverlay = "#FFFFFF";
}

// Element Plus 中性色 - 暗色模式
// 命名空间沿用历史名 VSCodeDark,实际值为 Element Plus 暗色色板,勿再增删 VS Code 色值
namespace VSCodeDark {
    // 文字色 - 暗色(次级/占位色冷调,贴合设计稿)
    constexpr const char* kTextPrimary = "#E5EAF3";
    constexpr const char* kTextRegular = "#CFD3DC";
    constexpr const char* kTextSecondary = "#8896AE";
    constexpr const char* kTextPlaceholder = "#6E7C99";
    constexpr const char* kTextDisabled = "#4C4D4F";

    // 边框色 - 深蓝调(对齐设计稿 #263146 暗卡边框系)
    constexpr const char* kBorderDarker = "#3C4C68";
    constexpr const char* kBorderDark = "#2E3C58";
    constexpr const char* kBorderBase = "#2C3A55";
    constexpr const char* kBorderLight = "#263146";
    constexpr const char* kBorderLighter = "#222E44";
    constexpr const char* kBorderExtraLight = "#1A2438";

    // 填充色 - 深蓝调(输入框/内嵌区)
    constexpr const char* kFillDarker = "#2C3A55";
    constexpr const char* kFillDark = "#222E44";
    constexpr const char* kFillBase = "#243049";
    constexpr const char* kFillLight = "#1B2335";
    constexpr const char* kFillLighter = "#131C2E";
    constexpr const char* kFillBlank = "#161E2E";

    // 背景色 - 深蓝黑(非中性灰黑):页面底 < 普通面 < 高层卡片
    constexpr const char* kBgWhite = "#161E2E";
    constexpr const char* kBgPage = "#0E1524";
    constexpr const char* kBgBase = "#161E2E";
    constexpr const char* kBgElevated = "#1A2438";
    constexpr const char* kBgOverlay = "#1A2438";
}

// Element Plus 尺寸与设计令牌规范
namespace Sizes {
    // 组件尺寸
    constexpr int kLarge = 40;
    constexpr int kDefault = 32;
    constexpr int kSmall = 24;

    // 圆角
    constexpr int kRadiusBase = 4;
    constexpr int kRadiusSmall = 2;
    // 消费级中等圆角:输入框、图标气泡、文件徽标等中小元素(V5 设计稿 ~12)
    constexpr int kRadiusMedium = 12;
    // 消费级大圆角:按钮、中型卡片(V5 设计稿 ~16)
    constexpr int kRadiusLarge = 16;
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

    // 字号(px)
    constexpr int kFontCaption = 12;
    constexpr int kFontBody = 14;
    constexpr int kFontSubtitle = 16;
    constexpr int kFontTitle = 18;
    constexpr int kFontH1 = 24;

    // 字重
    constexpr int kWeightRegular = 400;
    constexpr int kWeightMedium = 500;
    constexpr int kWeightDemiBold = 600;

    // 动效时长(ms)
    constexpr int kDurationFast = 100;
    constexpr int kDurationBase = 150;
    constexpr int kDurationSlow = 250;

    // 布局尺寸
    constexpr int kNavBarWidth = 74;
    constexpr int kTitleBarHeight = 40;
    constexpr int kSidebarWidth = 240;
    constexpr int kNavItemHeight = 44;
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
    Q_INVOKABLE QColor bgOverlay(bool isDark = false) const;
    Q_INVOKABLE QColor bgElevated(bool isDark = false) const;  // 新增：高层背景色

    // 尺寸
    Q_INVOKABLE int sizeLarge() const { return Sizes::kLarge; }
    Q_INVOKABLE int sizeDefault() const { return Sizes::kDefault; }
    Q_INVOKABLE int sizeSmall() const { return Sizes::kSmall; }

    Q_INVOKABLE int radiusBase() const { return Sizes::kRadiusBase; }
    Q_INVOKABLE int radiusSmall() const { return Sizes::kRadiusSmall; }
    Q_INVOKABLE int radiusMedium() const { return Sizes::kRadiusMedium; }
    Q_INVOKABLE int radiusLarge() const { return Sizes::kRadiusLarge; }
    Q_INVOKABLE int radiusRound() const { return Sizes::kRadiusRound; }

    Q_INVOKABLE int spaceLG() const { return Sizes::kSpaceLG; }
    Q_INVOKABLE int spaceMD() const { return Sizes::kSpaceMD; }
    Q_INVOKABLE int spaceSM() const { return Sizes::kSpaceSM; }

private:
    explicit ElementPlusColorProvider(QObject* parent = nullptr);
};

void RegisterElementPlusColors(QQmlEngine* engine);

} // namespace ElementPlusColors
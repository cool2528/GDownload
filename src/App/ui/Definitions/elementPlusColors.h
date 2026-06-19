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

// Ant Design 中性色 - 浅色模式
namespace AntDesignLight {
    // 文字色 - 模拟黑色透明度效果（Qt 不支持 #RRGGBBAA 格式）
    constexpr const char* kTextPrimary = "#262626";        // 模拟 rgba(0,0,0,0.85)
    constexpr const char* kTextRegular = "#595959";        // 模拟 rgba(0,0,0,0.65)
    constexpr const char* kTextSecondary = "#8C8C8C";      // 模拟 rgba(0,0,0,0.45)
    constexpr const char* kTextPlaceholder = "#BFBFBF";    // 模拟 rgba(0,0,0,0.25)
    constexpr const char* kTextDisabled = "#D9D9D9";       // 禁用文本更浅

    // 边框色
    constexpr const char* kBorderDarker = "#BFBFBF";
    constexpr const char* kBorderDark = "#D9D9D9";
    constexpr const char* kBorderBase = "#D9D9D9";         // Ant Design 标准边框
    constexpr const char* kBorderLight = "#E8E8E8";        // 调整为更明显的浅灰（原 #F0F0F0 太浅）
    constexpr const char* kBorderLighter = "#F0F0F0";
    constexpr const char* kBorderExtraLight = "#FAFAFA";

    // 填充色
    constexpr const char* kFillDarker = "#E6E6E6";
    constexpr const char* kFillDark = "#F0F0F0";
    constexpr const char* kFillBase = "#FAFAFA";
    constexpr const char* kFillLight = "#F5F5F5";
    constexpr const char* kFillLighter = "#FAFAFA";
    constexpr const char* kFillBlank = "#FFFFFF";

    // 背景色
    constexpr const char* kBgWhite = "#FFFFFF";            // 纯白高亮
    constexpr const char* kBgPage = "#F0F2F5";             // Ant Design 页面背景
    constexpr const char* kBgBase = "#FAFAFA";             // 卡片默认背景
    constexpr const char* kBgOverlay = "#FFFFFF";          // 浮层背景
}

// VS Code 中性色 - 暗色模式
namespace VSCodeDark {
    // 文字色 - VS Code 标准文本色
    constexpr const char* kTextPrimary = "#CCCCCC";        // VS Code 主要文本
    constexpr const char* kTextRegular = "#BBBBBB";        // 常规文本
    constexpr const char* kTextSecondary = "#969696";      // VS Code 次要文本
    constexpr const char* kTextPlaceholder = "#6A6A6A";    // placeholder
    constexpr const char* kTextDisabled = "#656565";       // VS Code 禁用文本

    // 边框色 - VS Code 层次色
    constexpr const char* kBorderDarker = "#474747";
    constexpr const char* kBorderDark = "#3E3E42";
    constexpr const char* kBorderBase = "#3E3E42";         // VS Code 标准边框
    constexpr const char* kBorderLight = "#2D2D30";
    constexpr const char* kBorderLighter = "#252526";
    constexpr const char* kBorderExtraLight = "#1E1E1E";

    // 填充色 - VS Code 背景层次
    constexpr const char* kFillDarker = "#474747";
    constexpr const char* kFillDark = "#3E3E42";
    constexpr const char* kFillBase = "#252526";
    constexpr const char* kFillLight = "#2D2D30";
    constexpr const char* kFillLighter = "#333333";
    constexpr const char* kFillBlank = "#1E1E1E";

    // 背景色 - VS Code 标准背景色
    constexpr const char* kBgWhite = "#1E1E1E";            // 编辑器背景（最深）
    constexpr const char* kBgPage = "#1E1E1E";             // 编辑器背景
    constexpr const char* kBgBase = "#252526";             // 侧边栏背景
    constexpr const char* kBgElevated = "#2D2D30";         // 活动项背景
    constexpr const char* kBgOverlay = "#333333";          // 活动栏背景
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
    Q_INVOKABLE QColor bgOverlay(bool isDark = false) const;
    Q_INVOKABLE QColor bgElevated(bool isDark = false) const;  // 新增：高层背景色

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

void RegisterElementPlusColors(QQmlEngine* engine);

} // namespace ElementPlusColors
#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QColor>
#include <QModelIndex>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QtQml/qqml.h>

// 视觉测试单例桩集合
//
// 设计动机:Phase 3(Task 6-8)视觉用例需要加载真实 QML 页面,这些页面通过
// `import gdl.sdk 1.0` 引用 GTheme / BrowserManager / SettingsManager 等单例。
// 但这些单例的真实 C++ 实现(BrowserManagerImpl / SettingsImpl / GTheme)符号
// 仅存在于 gdownload.exe,测试可执行文件 qml_ui_visual 无法链接。
// Phase 4(Task 9)将重构主项目为库以提供 Impl 符号;在那之前,本头文件提供
// 与 GTheme / BrowserManager 等 QML 接口兼容的最小桩对象,使页面可加载、
// 可渲染,从而产出有意义的视觉截图。
//
// 桩原则:
//   - 只暴露 QML 页面在视觉用例中实际引用的 Q_PROPERTY / Q_INVOKABLE
//   - 颜色/尺寸/字号令牌取 Element Plus 标准值,与 GTheme 真实实现一致
//   - 数据相关方法(GetActiveDownloadModel 等)返回 nullptr,页面以空状态渲染
//   - 副作用方法(ShowError / ParseShareUrl 等)空实现
//
// 注册:由 visual/main.cpp 的 TestSetup::qmlEngineAvailable 通过
// qmlRegisterSingletonInstance 注册到 "gdl.sdk" 模块,与生产环境单例名一致。

namespace gdl {
namespace tests {

// GTheme 桩:复刻 GTheme 全部 Q_PROPERTY,颜色值与 theme.cpp 实现一致
// theme 属性的 setter 命名沿用 QML_AUTO_PROPERTY 风格(Settheme/Gettheme)
// 参数类型用 int 以避免在测试桩中引入 GThemeType::ThemeMode 枚举依赖,
// QML 端写入 GThemeType.ThemeMode.Dark 时 Qt 元对象自动转 int
class TestGTheme : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool dark READ dark NOTIFY darkChanged)
	Q_PROPERTY(int theme READ Gettheme WRITE Settheme NOTIFY themeChanged)

	// 主色与状态色(浅暗色共用,Element Plus 标准)
	Q_PROPERTY(QColor primaryColor READ primaryColor CONSTANT)
	Q_PROPERTY(QColor successColor READ successColor CONSTANT)
	Q_PROPERTY(QColor warningColor READ warningColor CONSTANT)
	Q_PROPERTY(QColor dangerColor READ dangerColor CONSTANT)
	Q_PROPERTY(QColor infoColor READ infoColor CONSTANT)

	// 文字色(随 dark 切换)
	Q_PROPERTY(QColor textPrimary READ textPrimary NOTIFY darkChanged)
	Q_PROPERTY(QColor textRegular READ textRegular NOTIFY darkChanged)
	Q_PROPERTY(QColor textSecondary READ textSecondary NOTIFY darkChanged)
	Q_PROPERTY(QColor textPlaceholder READ textPlaceholder NOTIFY darkChanged)
	Q_PROPERTY(QColor textDisabled READ textDisabled NOTIFY darkChanged)

	// 边框色
	Q_PROPERTY(QColor borderBase READ borderBase NOTIFY darkChanged)
	Q_PROPERTY(QColor borderLight READ borderLight NOTIFY darkChanged)
	Q_PROPERTY(QColor borderLighter READ borderLighter NOTIFY darkChanged)

	// 填充色
	Q_PROPERTY(QColor fillBase READ fillBase NOTIFY darkChanged)
	Q_PROPERTY(QColor fillLight READ fillLight NOTIFY darkChanged)
	Q_PROPERTY(QColor fillLighter READ fillLighter NOTIFY darkChanged)

	// 背景色
	Q_PROPERTY(QColor bgWhite READ bgWhite NOTIFY darkChanged)
	Q_PROPERTY(QColor bgPage READ bgPage NOTIFY darkChanged)
	Q_PROPERTY(QColor bgBase READ bgBase NOTIFY darkChanged)
	Q_PROPERTY(QColor bgElevated READ bgElevated NOTIFY darkChanged)

	// 告警语义令牌(背景/边框/文字)
	Q_PROPERTY(QColor bgInfo READ bgInfo NOTIFY darkChanged)
	Q_PROPERTY(QColor bgSuccess READ bgSuccess NOTIFY darkChanged)
	Q_PROPERTY(QColor bgWarning READ bgWarning NOTIFY darkChanged)
	Q_PROPERTY(QColor bgDanger READ bgDanger NOTIFY darkChanged)
	Q_PROPERTY(QColor borderInfo READ borderInfo NOTIFY darkChanged)
	Q_PROPERTY(QColor borderSuccess READ borderSuccess NOTIFY darkChanged)
	Q_PROPERTY(QColor borderWarning READ borderWarning NOTIFY darkChanged)
	Q_PROPERTY(QColor borderDanger READ borderDanger NOTIFY darkChanged)
	Q_PROPERTY(QColor textInfo READ textInfo NOTIFY darkChanged)
	Q_PROPERTY(QColor textSuccess READ textSuccess NOTIFY darkChanged)
	Q_PROPERTY(QColor textWarning READ textWarning NOTIFY darkChanged)
	Q_PROPERTY(QColor textDanger READ textDanger NOTIFY darkChanged)

	// 尺寸令牌(CONSTANT,与主题无关)
	Q_PROPERTY(int sizeLarge READ sizeLarge CONSTANT)
	Q_PROPERTY(int sizeDefault READ sizeDefault CONSTANT)
	Q_PROPERTY(int sizeSmall READ sizeSmall CONSTANT)
	Q_PROPERTY(int radiusBase READ radiusBase CONSTANT)
	Q_PROPERTY(int radiusSmall READ radiusSmall CONSTANT)
	Q_PROPERTY(int spaceLG READ spaceLG CONSTANT)
	Q_PROPERTY(int spaceMD READ spaceMD CONSTANT)
	Q_PROPERTY(int spaceSM READ spaceSM CONSTANT)
	Q_PROPERTY(int spaceXS READ spaceXS CONSTANT)
	Q_PROPERTY(int spaceXL READ spaceXL CONSTANT)
	Q_PROPERTY(int space2XL READ space2XL CONSTANT)
	Q_PROPERTY(int space3XL READ space3XL CONSTANT)
	Q_PROPERTY(int radiusRound READ radiusRound CONSTANT)
	Q_PROPERTY(int radiusCircle READ radiusCircle CONSTANT)

	// 动效
	Q_PROPERTY(int durationFast READ durationFast CONSTANT)
	Q_PROPERTY(int durationBase READ durationBase CONSTANT)
	Q_PROPERTY(int durationSlow READ durationSlow CONSTANT)
	Q_PROPERTY(int easingStandard READ easingStandard CONSTANT)
	Q_PROPERTY(int easingEmphasized READ easingEmphasized CONSTANT)

	// 字号字重
	Q_PROPERTY(int fontCaption READ fontCaption CONSTANT)
	Q_PROPERTY(int fontBody READ fontBody CONSTANT)
	Q_PROPERTY(int fontSubtitle READ fontSubtitle CONSTANT)
	Q_PROPERTY(int fontTitle READ fontTitle CONSTANT)
	Q_PROPERTY(int fontH1 READ fontH1 CONSTANT)
	Q_PROPERTY(int weightRegular READ weightRegular CONSTANT)
	Q_PROPERTY(int weightMedium READ weightMedium CONSTANT)
	Q_PROPERTY(int weightDemiBold READ weightDemiBold CONSTANT)

	// 布局令牌
	Q_PROPERTY(int navBarWidth READ navBarWidth CONSTANT)
	Q_PROPERTY(int titleBarHeight READ titleBarHeight CONSTANT)
	Q_PROPERTY(int sidebarWidth READ sidebarWidth CONSTANT)
	Q_PROPERTY(int navItemHeight READ navItemHeight CONSTANT)

	// 阴影层级(深浅色不同)
	Q_PROPERTY(QVariantMap elevation1 READ elevation1 NOTIFY darkChanged)
	Q_PROPERTY(QVariantMap elevation2 READ elevation2 NOTIFY darkChanged)
	Q_PROPERTY(QVariantMap elevation3 READ elevation3 NOTIFY darkChanged)
	Q_PROPERTY(QVariantMap elevation4 READ elevation4 NOTIFY darkChanged)

   public:
	explicit TestGTheme(QObject* parent = nullptr) : QObject(parent) {}

	bool dark() const { return dark_; }

	// theme setter/getter:0=System,1=Light,2=Dark
	// QML_AUTO_PROPERTY 在生产 GTheme 上生成的 setter 名为 Settheme,这里保持一致
	void Settheme(int mode) {
		const bool new_dark = (mode == 2);
		if (new_dark != dark_) {
			dark_ = new_dark;
			Q_EMIT darkChanged();
			Q_EMIT themeChanged();
		}
	}
	int Gettheme() const { return dark_ ? 2 : 1; }

	// 主色与状态色
	QColor primaryColor() const { return QColor("#409EFF"); }
	QColor successColor() const { return QColor("#67C23A"); }
	QColor warningColor() const { return QColor("#E6A23C"); }
	QColor dangerColor() const { return QColor("#F56C6C"); }
	QColor infoColor() const { return QColor("#909399"); }

	// 文字色
	QColor textPrimary() const { return dark() ? QColor("#E5EAF3") : QColor("#303133"); }
	QColor textRegular() const { return dark() ? QColor("#CFD3DC") : QColor("#606266"); }
	QColor textSecondary() const { return dark() ? QColor("#A3A6AD") : QColor("#909399"); }
	QColor textPlaceholder() const { return dark() ? QColor("#8D9095") : QColor("#A8ABB2"); }
	QColor textDisabled() const { return dark() ? QColor("#6C6E72") : QColor("#C0C4CC"); }

	// 边框色
	QColor borderBase() const { return dark() ? QColor("#4C4D4F") : QColor("#DCDFE6"); }
	QColor borderLight() const { return dark() ? QColor("#414243") : QColor("#E4E7ED"); }
	QColor borderLighter() const { return dark() ? QColor("#363637") : QColor("#EBEEF5"); }

	// 填充色
	QColor fillBase() const { return dark() ? QColor("#303030") : QColor("#F0F2F5"); }
	QColor fillLight() const { return dark() ? QColor("#1D1D1D") : QColor("#F5F7FA"); }
	QColor fillLighter() const { return dark() ? QColor("#1D1D1D") : QColor("#FAFAFA"); }

	// 背景色
	QColor bgWhite() const { return dark() ? QColor("#1D1D1D") : QColor("#FFFFFF"); }
	QColor bgPage() const { return dark() ? QColor("#252526") : QColor("#F2F3F5"); }
	QColor bgBase() const { return dark() ? QColor("#1D1D1D") : QColor("#FFFFFF"); }
	QColor bgElevated() const { return dark() ? QColor("#252526") : QColor("#FFFFFF"); }

	// 告警语义令牌
	QColor bgInfo() const { return dark() ? QColor("#1D1D1D") : QColor("#F4F4F5"); }
	QColor bgSuccess() const { return dark() ? QColor("#16281B") : QColor("#F0F9EB"); }
	QColor bgWarning() const { return dark() ? QColor("#292418") : QColor("#FDF6EC"); }
	QColor bgDanger() const { return dark() ? QColor("#2B1A1A") : QColor("#FEF0F0"); }
	QColor borderInfo() const { return dark() ? QColor("#313134") : QColor("#E9E9EB"); }
	QColor borderSuccess() const { return dark() ? QColor("#1B3A23") : QColor("#E1F3D8"); }
	QColor borderWarning() const { return dark() ? QColor("#3A2F18") : QColor("#FAECD8"); }
	QColor borderDanger() const { return dark() ? QColor("#3A2222") : QColor("#FDE2E2"); }
	QColor textInfo() const { return dark() ? QColor("#A3A6AD") : QColor("#909399"); }
	QColor textSuccess() const { return QColor("#67C23A"); }
	QColor textWarning() const { return QColor("#E6A23C"); }
	QColor textDanger() const { return QColor("#F56C6C"); }

	// 尺寸(对齐 ElementPlusColors::Sizes 常量)
	int sizeLarge() const { return 40; }
	int sizeDefault() const { return 32; }
	int sizeSmall() const { return 24; }
	int radiusBase() const { return 4; }
	int radiusSmall() const { return 2; }
	int spaceLG() const { return 16; }
	int spaceMD() const { return 12; }
	int spaceSM() const { return 8; }
	int spaceXS() const { return 4; }
	int spaceXL() const { return 20; }
	int space2XL() const { return 24; }
	int space3XL() const { return 32; }
	int radiusRound() const { return 999; }
	int radiusCircle() const { return 999; }

	// 动效
	int durationFast() const { return 100; }
	int durationBase() const { return 150; }
	int durationSlow() const { return 250; }
	// QEasingCurve::OutCubic = 16,OutQuint = 22
	int easingStandard() const { return 16; }
	int easingEmphasized() const { return 22; }

	// 字号字重
	int fontCaption() const { return 12; }
	int fontBody() const { return 14; }
	int fontSubtitle() const { return 16; }
	int fontTitle() const { return 18; }
	int fontH1() const { return 24; }
	int weightRegular() const { return 400; }
	int weightMedium() const { return 500; }
	int weightDemiBold() const { return 600; }

	// 布局
	int navBarWidth() const { return 74; }
	int titleBarHeight() const { return 40; }
	int sidebarWidth() const { return 240; }
	int navItemHeight() const { return 44; }

	// 阴影层级
	QVariantMap elevation1() const { return makeElevation(dark() ? QColor(0, 0, 0, 80) : QColor(0, 0, 0, 16), 4, 1); }
	QVariantMap elevation2() const { return makeElevation(dark() ? QColor(0, 0, 0, 120) : QColor(0, 0, 0, 24), 8, 2); }
	QVariantMap elevation3() const { return makeElevation(dark() ? QColor(0, 0, 0, 150) : QColor(0, 0, 0, 32), 12, 4); }
	QVariantMap elevation4() const { return makeElevation(dark() ? QColor(0, 0, 0, 180) : QColor(0, 0, 0, 40), 16, 8); }

	// 主色色阶:严格对齐 Element Plus Primary light3/5/7/8/9
	// GButton.qml 在 hover/checked 态调用 GTheme.primaryLight(3/5/9) 取浅色,
	// 桩缺这些 Q_INVOKABLE 会导致 GButton.colors 求值抛 TypeError,渲染空白。
	Q_INVOKABLE QColor primaryLight(int level = 3) const {
		switch (level) {
			case 3: return QColor("#79BBFF");
			case 5: return QColor("#A0CFFF");
			case 7: return QColor("#C6E2FF");
			case 8: return QColor("#D9ECFF");
			case 9: return QColor("#ECF5FF");
			default: return primaryColor();
		}
	}
	Q_INVOKABLE QColor successLight(int level = 3) const {
		switch (level) {
			case 3: return QColor("#95D475");
			case 5: return QColor("#B3E19D");
			case 7: return QColor("#D1EDC4");
			case 8: return QColor("#E1F3D8");
			case 9: return QColor("#F0F9EB");
			default: return successColor();
		}
	}
	Q_INVOKABLE QColor warningLight(int level = 3) const {
		switch (level) {
			case 3: return QColor("#EEBE77");
			case 5: return QColor("#F3D19E");
			case 7: return QColor("#F8D3A0");
			case 8: return QColor("#FAECD8");
			case 9: return QColor("#FDF6EC");
			default: return warningColor();
		}
	}
	Q_INVOKABLE QColor dangerLight(int level = 3) const {
		switch (level) {
			case 3: return QColor("#F89898");
			case 5: return QColor("#FAB6B6");
			case 7: return QColor("#FBC4C4");
			case 8: return QColor("#FDE2E2");
			case 9: return QColor("#FEF0F0");
			default: return dangerColor();
		}
	}
	Q_INVOKABLE QColor infoLight(int level = 3) const {
		switch (level) {
			case 3: return QColor("#B1B3B8");
			case 5: return QColor("#C8C9CC");
			case 7: return QColor("#DEDEDF");
			case 8: return QColor("#E9E9EB");
			case 9: return QColor("#F4F4F5");
			default: return infoColor();
		}
	}

   Q_SIGNALS:
	void darkChanged();
	void themeChanged();

   private:
	static QVariantMap makeElevation(const QColor& color, int blur, int offset_y) {
		QVariantMap m;
		m.insert("color", color);
		m.insert("blur", blur);
		m.insert("offsetX", 0);
		m.insert("offsetY", offset_y);
		m.insert("spread", 0);
		return m;
	}
	bool dark_ = false;
};

// BrowserManager 桩:仅提供 DownloadPageView 在 QML 中调用的 3 个 Q_INVOKABLE
// 返回 nullptr 使 GDownloadViewPage 以空模型渲染
class TestBrowserManager : public QObject {
	Q_OBJECT
   public:
	explicit TestBrowserManager(QObject* parent = nullptr) : QObject(parent) {}
	Q_INVOKABLE QObject* GetActiveDownloadModel() { return nullptr; }
	Q_INVOKABLE QObject* GetWaitingDownloadModel() { return nullptr; }
	Q_INVOKABLE QObject* GetStopedDownloadModel() { return nullptr; }
};

// SettingsManager 桩:提供 mainWindow / TitleBar / NetDiskPageView 引用的 qXxx 属性
// 命名沿用 SETTING_PROPERTY 宏生成的 q 前缀
class TestSettingsManager : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool qRememberWindowPosition MEMBER remember_window_position_ CONSTANT)
	Q_PROPERTY(QSize qWindowSize MEMBER window_size_ CONSTANT)
	Q_PROPERTY(QPoint qWindowPosition MEMBER window_position_ CONSTANT)
	Q_PROPERTY(QString qBaiduPanCookies MEMBER baidu_pan_cookies_ CONSTANT)
	Q_PROPERTY(bool qShowCloseConfirm MEMBER show_close_confirm_ CONSTANT)
	Q_PROPERTY(bool qCloseToTray MEMBER close_to_tray_ CONSTANT)
	Q_PROPERTY(QString qTheme MEMBER theme_ CONSTANT)
	Q_PROPERTY(QString qLanguage MEMBER language_ CONSTANT)

   public:
	explicit TestSettingsManager(QObject* parent = nullptr) : QObject(parent) {}

   private:
	bool remember_window_position_ = false;
	QSize window_size_ = QSize(1280, 720);
	QPoint window_position_ = QPoint(100, 100);
	QString baidu_pan_cookies_;
	bool show_close_confirm_ = true;
	bool close_to_tray_ = false;
	QString theme_ = QStringLiteral("light");
	QString language_ = QStringLiteral("en_US");
};

// ToastManager 桩:ShowXxx 空实现,避免 QML 调用时缺少方法
class TestToastManager : public QObject {
	Q_OBJECT
   public:
	explicit TestToastManager(QObject* parent = nullptr) : QObject(parent) {}
	Q_INVOKABLE void ShowError(const QString& msg) { Q_UNUSED(msg); }
	Q_INVOKABLE void ShowSuccess(const QString& msg) { Q_UNUSED(msg); }
	Q_INVOKABLE void ShowWarning(const QString& msg) { Q_UNUSED(msg); }
	Q_INVOKABLE void ShowInfo(const QString& msg) { Q_UNUSED(msg); }
};

// NetWorkDiskManager 桩:ParseShareUrl 空实现
class TestNetWorkDiskManager : public QObject {
	Q_OBJECT
   public:
	explicit TestNetWorkDiskManager(QObject* parent = nullptr) : QObject(parent) {}
	Q_INVOKABLE void ParseShareUrl(const QString& url) { Q_UNUSED(url); }
};

// UpdateManager 桩:空对象,QML 引用其存在即可
class TestUpdateManager : public QObject {
	Q_OBJECT
   public:
	explicit TestUpdateManager(QObject* parent = nullptr) : QObject(parent) {}
};

// UtilsToolsManager 桩:HideMacOsxWindowStandardButtons 等空实现
class TestUtilsToolsManager : public QObject {
	Q_OBJECT
   public:
	explicit TestUtilsToolsManager(QObject* parent = nullptr) : QObject(parent) {}
	Q_INVOKABLE void HideMacOsxWindowStandardButtons(QObject* window) { Q_UNUSED(window); }
	Q_INVOKABLE bool SetClipboardText(const QString& text) {
		Q_UNUSED(text);
		return false;
	}
	Q_INVOKABLE QString Version() const { return QStringLiteral("test-stub"); }
};

// LanguageManager 桩:空对象
class TestLanguageManager : public QObject {
	Q_OBJECT
   public:
	explicit TestLanguageManager(QObject* parent = nullptr) : QObject(parent) {}
};

// FolderHistoryModel 桩:QAbstractListModel 子类,提供 maxHistoryCount 属性
// NavigatorView 内部 Component 引用 TaskDialogPage -> ... -> FolderSelector,
// FolderSelector 声明 `property FolderHistoryModel historyModel: FolderHistoryModel{}`,
// 缺该类型会导致 NavigatorView 整体加载失败,截图空白。
// 桩只暴露 QML 端实际引用的 maxHistoryCount 属性(默认 10),列表数据为空。
class TestFolderHistoryModel : public QAbstractListModel {
	Q_OBJECT
	Q_PROPERTY(int maxHistoryCount READ maxHistoryCount WRITE setMaxHistoryCount NOTIFY maxHistoryCountChanged)
   public:
	explicit TestFolderHistoryModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}
	int maxHistoryCount() const { return max_history_count_; }
	void setMaxHistoryCount(int count) {
		if (count != max_history_count_) {
			max_history_count_ = count;
			Q_EMIT maxHistoryCountChanged();
		}
	}
	Q_INVOKABLE void addPath(const QString& path) { Q_UNUSED(path); }
	Q_INVOKABLE void removePath(int index) { Q_UNUSED(index); }
	Q_INVOKABLE void clear() {}
	int rowCount(const QModelIndex& parent = QModelIndex()) const override {
		Q_UNUSED(parent);
		return 0;
	}
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
		Q_UNUSED(index);
		Q_UNUSED(role);
		return QVariant();
	}
   Q_SIGNALS:
	void maxHistoryCountChanged();

   private:
	int max_history_count_ = 10;
};

}  // namespace tests
}  // namespace gdl

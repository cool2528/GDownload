#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QColor>
#include <QList>
#include <QModelIndex>
#include <QPair>
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
//   - 数据相关方法(GetActiveDownloadModel 等)返回视觉样例模型,页面以任务卡片态渲染
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
	Q_PROPERTY(QColor primaryColor READ primaryColor NOTIFY darkChanged)
	Q_PROPERTY(QColor brandHover READ brandHover NOTIFY darkChanged)
	Q_PROPERTY(QColor brandPressed READ brandPressed NOTIFY darkChanged)
	Q_PROPERTY(QColor successColor READ successColor NOTIFY darkChanged)
	Q_PROPERTY(QColor warningColor READ warningColor NOTIFY darkChanged)
	Q_PROPERTY(QColor dangerColor READ dangerColor NOTIFY darkChanged)
	Q_PROPERTY(QColor infoColor READ infoColor NOTIFY darkChanged)

	// 文字色(随 dark 切换)
	Q_PROPERTY(QColor textPrimary READ textPrimary NOTIFY darkChanged)
	Q_PROPERTY(QColor textRegular READ textRegular NOTIFY darkChanged)
	Q_PROPERTY(QColor textSecondary READ textSecondary NOTIFY darkChanged)
	Q_PROPERTY(QColor textPlaceholder READ textPlaceholder NOTIFY darkChanged)
	Q_PROPERTY(QColor textDisabled READ textDisabled NOTIFY darkChanged)
	Q_PROPERTY(QColor textInverse READ textInverse CONSTANT)

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
	Q_PROPERTY(QColor bgOverlay READ bgOverlay NOTIFY darkChanged)
	Q_PROPERTY(QColor surfaceBase READ surfaceBase NOTIFY darkChanged)
	Q_PROPERTY(QColor surfaceElevated READ surfaceElevated NOTIFY darkChanged)
	Q_PROPERTY(QColor focusRing READ focusRing NOTIFY darkChanged)
	Q_PROPERTY(QColor overlayScrim READ overlayScrim NOTIFY darkChanged)

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
	Q_PROPERTY(int radiusMedium READ radiusMedium CONSTANT)
	Q_PROPERTY(int radiusLarge READ radiusLarge CONSTANT)
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
	QColor primaryColor() const { return dark() ? QColor("#5AAEFF") : QColor("#409EFF"); }
	QColor brandHover() const { return dark() ? QColor("#73B7FF") : QColor("#2F8EEA"); }
	QColor brandPressed() const { return dark() ? QColor("#3F95EC") : QColor("#237FD6"); }
	QColor successColor() const { return dark() ? QColor("#4ADE80") : QColor("#34C759"); }
	QColor warningColor() const { return dark() ? QColor("#FBBF24") : QColor("#F5A524"); }
	QColor dangerColor() const { return dark() ? QColor("#FB7185") : QColor("#F05252"); }
	QColor infoColor() const { return dark() ? QColor("#94A3B8") : QColor("#909399"); }

	// 文字色
	QColor textPrimary() const { return dark() ? QColor("#F3F7FF") : QColor("#1F2937"); }
	QColor textRegular() const { return dark() ? QColor("#D7E0EF") : QColor("#414C5E"); }
	QColor textSecondary() const { return dark() ? QColor("#A8B4C7") : QColor("#596579"); }
	QColor textPlaceholder() const { return dark() ? QColor("#8090AA") : QColor("#7B879B"); }
	QColor textDisabled() const { return dark() ? QColor("#62708A") : QColor("#A8B1C0"); }
	QColor textInverse() const { return QColor("#FFFFFF"); }

	// 边框色
	QColor borderBase() const { return dark() ? QColor("#34435E") : QColor("#CBD6E5"); }
	QColor borderLight() const { return dark() ? QColor("#2B3951") : QColor("#DCE5F0"); }
	QColor borderLighter() const { return dark() ? QColor("#233149") : QColor("#E7EDF5"); }

	// 填充色
	QColor fillBase() const { return dark() ? QColor("#243049") : QColor("#EDF2FA"); }
	QColor fillLight() const { return dark() ? QColor("#1B263A") : QColor("#F2F6FC"); }
	QColor fillLighter() const { return dark() ? QColor("#131C2E") : QColor("#F7FAFE"); }

	// 背景色
	QColor bgWhite() const { return dark() ? QColor("#101827") : QColor("#FFFFFF"); }
	QColor bgPage() const { return dark() ? QColor("#080D18") : QColor("#F3F6FB"); }
	QColor bgBase() const { return dark() ? QColor("#101827") : QColor("#FFFFFF"); }
	QColor bgElevated() const { return dark() ? QColor("#151F31") : QColor("#FFFFFF"); }
	QColor bgOverlay() const { return bgElevated(); }
	QColor surfaceBase() const { return bgBase(); }
	QColor surfaceElevated() const { return bgElevated(); }
	QColor focusRing() const { return primaryColor(); }
	QColor overlayScrim() const { return dark() ? QColor(0, 0, 0, 153) : QColor(8, 13, 24, 82); }

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
	int radiusMedium() const { return 12; }
	int radiusLarge() const { return 16; }
	int spaceLG() const { return 16; }
	int spaceMD() const { return 12; }
	int spaceSM() const { return 8; }
	int spaceXS() const { return 4; }
	int spaceXL() const { return 20; }
	int space2XL() const { return 24; }
	int space3XL() const { return 32; }
	int radiusRound() const { return 20; }
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

// 下载任务视觉模型桩:给 DownloadPageView 截图提供稳定样例数据
class TestDownloadTaskModel : public QAbstractListModel {
	Q_OBJECT
	Q_PROPERTY(int count READ count NOTIFY countChanged)
   public:
	enum Roles {
		kTaskId = Qt::UserRole + 1,
		kTaskState,
		kTaskFileName,
		kTaskSavePath,
		kTaskTotalSize,
		kTaskCurrentSize,
		kTaskDownloadSpeed,
		kTaskProgress,
		kTaskRemainingTime,
		kTaskConnections,
		kTaskDownloadLink,
		kTaskErrorCode,
		kTaskErrorMessage,
	};

	explicit TestDownloadTaskModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}
	int count() const { return rows_.size(); }

	int rowCount(const QModelIndex& parent = QModelIndex()) const override {
		if (parent.isValid()) return 0;
		return rows_.size();
	}

	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid() || index.row() < 0 || index.row() >= rows_.size()) return {};
		return rows_.at(index.row()).value(QString::fromUtf8(roleNames().value(role)));
	}

	QHash<int, QByteArray> roleNames() const override {
		return {{kTaskId, "taskId"},
			{kTaskState, "taskState"},
			{kTaskFileName, "fileName"},
			{kTaskSavePath, "savePath"},
			{kTaskTotalSize, "totalSize"},
			{kTaskCurrentSize, "currentSize"},
			{kTaskDownloadSpeed, "downloadSpeed"},
			{kTaskProgress, "progress"},
			{kTaskRemainingTime, "remainingTime"},
			{kTaskConnections, "connections"},
			{kTaskDownloadLink, "downloadLink"},
			{kTaskErrorCode, "errorCode"},
			{kTaskErrorMessage, "errorMessage"}};
	}

	void setRows(const QList<QVariantMap>& rows) {
		beginResetModel();
		rows_ = rows;
		endResetModel();
		Q_EMIT countChanged();
	}

   Q_SIGNALS:
	void countChanged();

   private:
	QList<QVariantMap> rows_;
};

// BrowserManager 桩:视觉用例返回稳定样例模型,使下载页截图覆盖任务卡片态
class TestBrowserManager : public QObject {
	Q_OBJECT
   public:
	explicit TestBrowserManager(QObject* parent = nullptr) : QObject(parent) {
		active_model_.setRows({
			{{"taskId", "active-1"}, {"taskState", 1}, {"fileName", "Ubuntu Desktop 26.04.iso"},
			 {"savePath", "C:/Downloads/Linux/Ubuntu Desktop 26.04.iso"}, {"totalSize", "4.1 GB"},
			 {"currentSize", "2.8 GB"}, {"downloadSpeed", "18.4 MB/s"}, {"progress", 68},
			 {"remainingTime", "12 min"}, {"connections", 16},
			 {"downloadLink", "https://releases.ubuntu.com/26.04/ubuntu-desktop.iso"}},
			{{"taskId", "active-2"}, {"taskState", 1}, {"fileName", "gdownload-assets.zip"},
			 {"savePath", "C:/Downloads/gdownload-assets.zip"}, {"totalSize", "820 MB"},
			 {"currentSize", "610 MB"}, {"downloadSpeed", "6.4 MB/s"}, {"progress", 74},
			 {"remainingTime", "3 min"}, {"connections", 8},
			 {"downloadLink", "https://example.com/gdownload-assets.zip"}},
			{{"taskId", "active-3"}, {"taskState", 0}, {"fileName", "DesignCourse-Part-12.mp4"},
			 {"savePath", "C:/Downloads/Courses/DesignCourse-Part-12.mp4"}, {"totalSize", "1.8 GB"},
			 {"currentSize", "420 MB"}, {"downloadSpeed", "0 B/s"}, {"progress", 23},
			 {"remainingTime", "Paused"}, {"connections", 0},
			 {"downloadLink", "https://example.com/design-course-part-12.mp4"}},
		});

		waiting_model_.setRows({
			{{"taskId", "waiting-1"}, {"taskState", 0}, {"fileName", "Qt Documentation Offline.7z"},
			 {"savePath", "C:/Downloads/Qt Documentation Offline.7z"}, {"totalSize", "640 MB"},
			 {"currentSize", "0 B"}, {"downloadSpeed", "0 B/s"}, {"progress", 0},
			 {"remainingTime", "Waiting"}, {"connections", 0}, {"downloadLink", "https://example.com/qt-docs.7z"}},
		});

		stopped_model_.setRows({
			{{"taskId", "stopped-1"}, {"taskState", 0}, {"fileName", "GDownload-Installer.exe"},
			 {"savePath", "C:/Downloads/GDownload-Installer.exe"}, {"totalSize", "86 MB"},
			 {"currentSize", "86 MB"}, {"downloadSpeed", "0 B/s"}, {"progress", 100},
			 {"remainingTime", "Completed"}, {"connections", 0},
			 {"downloadLink", "https://example.com/GDownload-Installer.exe"}, {"errorCode", ""},
			 {"errorMessage", ""}},
			{{"taskId", "failed-1"}, {"taskState", 4}, {"fileName", "Linux-Image.iso"},
			 {"savePath", "C:/Downloads/Linux-Image.iso"}, {"totalSize", "2.4 GB"},
			 {"currentSize", "312 MB"}, {"downloadSpeed", "0 B/s"}, {"progress", 13},
			 {"remainingTime", "Stopped"}, {"connections", 0},
			 {"downloadLink", "https://example.com/Linux-Image.iso"}, {"errorCode", "3"},
			 {"errorMessage", "Resource not found"}},
		});
	}

	Q_INVOKABLE QObject* GetActiveDownloadModel() { return &active_model_; }
	Q_INVOKABLE QObject* GetWaitingDownloadModel() { return &waiting_model_; }
	Q_INVOKABLE QObject* GetStopedDownloadModel() { return &stopped_model_; }
	Q_INVOKABLE void SyncTrackersServerlist() {}
	Q_INVOKABLE bool RetryTask(const QString& gid) {
		Q_UNUSED(gid);
		return true;
	}

   Q_SIGNALS:
	void sigTrackerUpdateStatus(const QString& status);

   private:
	TestDownloadTaskModel active_model_{this};
	TestDownloadTaskModel waiting_model_{this};
	TestDownloadTaskModel stopped_model_{this};
};

// SettingsManager 桩:提供 mainWindow / TitleBar / NetDiskPageView 及全部 Settings 页面
// 引用的 qXxx 属性与 SetXxx 方法。命名沿用 SETTING_PROPERTY 宏生成的 q 前缀。
// Task 7(设置页视觉用例)覆盖 14 个 SettingPage,这些页面读取 30+ 个 qXxx 属性
// 并调用对应 SetXxx Q_INVOKABLE 保存改动。桩提供全部属性(合理默认值,使页面
// 渲染出有意义的视觉内容)与全部 SetXxx(空实现,视觉用例不验证保存行为)。
// Phase 3b Task 8 追加 Ed2kSettingPage 的 8 个 qEd2kXxx 属性 + 8 个 SetEd2kXxx,
// 使新增的 ed2k 设置页视觉用例(第 15 页)可加载、可渲染。
class TestSettingsManager : public QObject {
	Q_OBJECT
	// 主窗口/通用(已存在属性沿用 Task 6)
	Q_PROPERTY(bool qRememberWindowPosition MEMBER remember_window_position_ CONSTANT)
	Q_PROPERTY(QSize qWindowSize MEMBER window_size_ CONSTANT)
	Q_PROPERTY(QPoint qWindowPosition MEMBER window_position_ CONSTANT)
	Q_PROPERTY(QString qBaiduPanCookies MEMBER baidu_pan_cookies_ CONSTANT)
	Q_PROPERTY(bool qShowCloseConfirm MEMBER show_close_confirm_ CONSTANT)
	Q_PROPERTY(bool qCloseToTray MEMBER close_to_tray_ CONSTANT)
	Q_PROPERTY(QString qTheme MEMBER theme_ CONSTANT)
	Q_PROPERTY(QString qLanguage MEMBER language_ CONSTANT)

	// BasicSettingPage:应用行为 / 下载路径 / 网络代理 / 剪贴板监听
	Q_PROPERTY(bool qEnableAutoUpdate MEMBER enable_auto_update_ CONSTANT)
	Q_PROPERTY(bool qEnableGithubAccelerate MEMBER enable_github_accelerate_ CONSTANT)
	Q_PROPERTY(bool qAutoStart MEMBER auto_start_ CONSTANT)
	Q_PROPERTY(bool qAutoResumeTask MEMBER auto_resume_task_ CONSTANT)
	Q_PROPERTY(QString qDir MEMBER dir_ CONSTANT)
	Q_PROPERTY(bool qEnableGlobalProxy MEMBER enable_global_proxy_ CONSTANT)
	Q_PROPERTY(QString qGlobalProxy MEMBER global_proxy_ CONSTANT)
	Q_PROPERTY(bool qListenClipboard MEMBER listen_clipboard_ CONSTANT)

	// Aria2RpcSettingPage:RPC 端口与密钥
	Q_PROPERTY(int qRpcListenPort MEMBER rpc_listen_port_ CONSTANT)
	Q_PROPERTY(QString qRpcSecret MEMBER rpc_secret_ CONSTANT)

	// BitTorrentAdvancedSettingPage:BT 高级
	Q_PROPERTY(bool qEnableDht MEMBER enable_dht_ CONSTANT)
	Q_PROPERTY(int qBtMaxPeers MEMBER bt_max_peers_ CONSTANT)
	Q_PROPERTY(bool qBtRequireCrypto MEMBER bt_require_crypto_ CONSTANT)

	// ConnectionPerformanceSettingPage:并发与分片
	Q_PROPERTY(int qMaxConcurrentDownloads MEMBER max_concurrent_downloads_ CONSTANT)
	Q_PROPERTY(int qMaxConnectionPerServer MEMBER max_connection_per_server_ CONSTANT)
	Q_PROPERTY(int qSplit MEMBER split_ CONSTANT)
	Q_PROPERTY(int qMinSplitSize MEMBER min_split_size_ CONSTANT)

	// PostDownloadActionsSettingPage:完成/错误/启动动作
	Q_PROPERTY(int qOnCompleteAction MEMBER on_complete_action_ CONSTANT)
	Q_PROPERTY(QString qCustomCompleteCommand MEMBER custom_complete_command_ CONSTANT)
	Q_PROPERTY(int qOnErrorAction MEMBER on_error_action_ CONSTANT)
	Q_PROPERTY(QString qCustomErrorCommand MEMBER custom_error_command_ CONSTANT)
	Q_PROPERTY(int qOnStartAction MEMBER on_start_action_ CONSTANT)

	// SpeedControlSettingPage:全局上下行限速
	Q_PROPERTY(int qMaxOverallDownloadLimit MEMBER max_overall_download_limit_ CONSTANT)
	Q_PROPERTY(int qMaxOverallUploadLimit MEMBER max_overall_upload_limit_ CONSTANT)
	Q_PROPERTY(int qLowestSpeedLimit MEMBER lowest_speed_limit_ CONSTANT)

	// TimeoutRetrySettingPage:超时与重试
	Q_PROPERTY(int qTimeout MEMBER timeout_ CONSTANT)
	Q_PROPERTY(int qConnectTimeout MEMBER connect_timeout_ CONSTANT)
	Q_PROPERTY(int qMaxTries MEMBER max_tries_ CONSTANT)
	Q_PROPERTY(int qRetryWait MEMBER retry_wait_ CONSTANT)

	// TrackerServerSettingPage:tracker 源
	// qTrackerSourceNames 必须为合法 JSON 字符串,页面 Component.onCompleted
	// 调 JSON.parse 解析;空数组 "[]" 保证不抛 SyntaxError。
	Q_PROPERTY(QString qTrackerSourceNames MEMBER tracker_source_names_ CONSTANT)
	Q_PROPERTY(bool qEnableTrackerSourceAutoUpdate MEMBER enable_tracker_source_auto_update_ CONSTANT)

	// UserAgentSettingPage:UA
	Q_PROPERTY(QString qUserAgent MEMBER user_agent_ CONSTANT)

	// Ed2kSettingPage(Phase 3b Task 8):eD2k 身份/网络接入/性能
	Q_PROPERTY(QString qEd2kNickname MEMBER ed2k_nickname_ CONSTANT)
	Q_PROPERTY(int qEd2kTcpPort MEMBER ed2k_tcp_port_ CONSTANT)
	Q_PROPERTY(int qEd2kUdpPort MEMBER ed2k_udp_port_ CONSTANT)
	Q_PROPERTY(bool qEd2kEnableKad MEMBER ed2k_enable_kad_ CONSTANT)
	Q_PROPERTY(bool qEd2kEnableObfuscation MEMBER ed2k_enable_obfuscation_ CONSTANT)
	Q_PROPERTY(bool qEd2kAutoConnect MEMBER ed2k_auto_connect_ CONSTANT)
	Q_PROPERTY(int qEd2kMaxConcurrentTasks MEMBER ed2k_max_concurrent_tasks_ CONSTANT)
	Q_PROPERTY(QString qEd2kServerMetUrl MEMBER ed2k_server_met_url_ CONSTANT)
	Q_PROPERTY(QString qEd2kNodesDatUrl MEMBER ed2k_nodes_dat_url_ CONSTANT)

   public:
	explicit TestSettingsManager(QObject* parent = nullptr) : QObject(parent) {}

	// BasicSettingPage setters(空实现,视觉用例不验证保存)
	Q_INVOKABLE void SetEnableAutoUpdate(bool v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetEnableGithubAccelerate(bool v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAutoStart(bool v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetRememberWindowPosition(bool v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAria2AutoResumeTask(bool v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetShowCloseConfirm(bool v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAria2Dir(const QString& v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetEnableGlobalProxy(bool v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAria2GlobalProxy(const QString& v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetListenClipboard(bool v) { Q_UNUSED(v); }

	// Aria2RpcSettingPage setters + GenerateRpcSecret
	Q_INVOKABLE void SetRpcListenPort(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetRpcSecret(const QString& v) { Q_UNUSED(v); }
	Q_INVOKABLE QString GenerateRpcSecret() { return QStringLiteral("test-secret-stub-0123456789abcdef"); }

	// BaiduCookieSettingPage setter
	Q_INVOKABLE void SetBaiduPanCookies(const QString& v) { Q_UNUSED(v); }

	// BitTorrentAdvancedSettingPage setters
	Q_INVOKABLE void SetAria2EnableDht(bool v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAria2BtMaxPeers(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAria2BtRequireCrypto(bool v) { Q_UNUSED(v); }

	// ConnectionPerformanceSettingPage setters
	Q_INVOKABLE void SetAria2MaxConcurrentDownloads(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAria2MaxConnectionPerServer(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAria2Split(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAria2MinSplitSize(int v) { Q_UNUSED(v); }

	// PostDownloadActionsSettingPage setters
	Q_INVOKABLE void SetOnCompleteAction(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetCustomCompleteCommand(const QString& v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetOnErrorAction(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetCustomErrorCommand(const QString& v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetOnStartAction(int v) { Q_UNUSED(v); }

	// SpeedControlSettingPage setters
	Q_INVOKABLE void SetAria2MaxOverallDownloadLimit(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAria2MaxOverallUploadLimit(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAria2LowestSpeedLimit(int v) { Q_UNUSED(v); }

	// TimeoutRetrySettingPage setters
	Q_INVOKABLE void SetAria2Timeout(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAria2ConnectTimeout(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAria2MaxTries(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetAria2RetryWait(int v) { Q_UNUSED(v); }

	// TrackerServerSettingPage setters
	Q_INVOKABLE void SetTrackerSourceNames(const QString& v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetEnableTrackerSourceAutoUpdate(bool v) { Q_UNUSED(v); }

	// UserAgentSettingPage setter
	Q_INVOKABLE QString GetDefaultBrowserUserAgent() const {
		return QStringLiteral(
			"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
			"Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0");
	}
	Q_INVOKABLE void SetAria2UserAgent(const QString& v) { Q_UNUSED(v); }

	// Ed2kSettingPage setters(空实现,视觉用例不验证保存)
	Q_INVOKABLE void SetEd2kNickname(const QString& v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetEd2kTcpPort(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetEd2kUdpPort(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetEd2kEnableKad(bool v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetEd2kEnableObfuscation(bool v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetEd2kAutoConnect(bool v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetEd2kMaxConcurrentTasks(int v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetEd2kServerMetUrl(const QString& v) { Q_UNUSED(v); }
	Q_INVOKABLE void SetEd2kNodesDatUrl(const QString& v) { Q_UNUSED(v); }

   private:
	bool remember_window_position_ = false;
	QSize window_size_ = QSize(1280, 720);
	QPoint window_position_ = QPoint(100, 100);
	QString baidu_pan_cookies_;
	bool show_close_confirm_ = true;
	bool close_to_tray_ = false;
	QString theme_ = QStringLiteral("light");
	QString language_ = QStringLiteral("en_US");

	// BasicSettingPage 默认值(开关为合理开启态,使页面视觉更饱满)
	bool enable_auto_update_ = true;
	bool enable_github_accelerate_ = true;
	bool auto_start_ = false;
	bool auto_resume_task_ = true;
	QString dir_ = QStringLiteral("C:/Downloads");
	bool enable_global_proxy_ = false;
	QString global_proxy_;
	bool listen_clipboard_ = true;

	// Aria2RpcSettingPage(端口 6800 为 aria2 默认 RPC 端口)
	int rpc_listen_port_ = 6800;
	QString rpc_secret_ = QStringLiteral("test-secret-stub");

	// BitTorrentAdvancedSettingPage
	bool enable_dht_ = true;
	int bt_max_peers_ = 55;
	bool bt_require_crypto_ = false;

	// ConnectionPerformanceSettingPage(aria2 推荐默认)
	int max_concurrent_downloads_ = 5;
	int max_connection_per_server_ = 16;
	int split_ = 64;
	int min_split_size_ = 20;

	// PostDownloadActionsSettingPage(0 = 无动作)
	int on_complete_action_ = 0;
	QString custom_complete_command_;
	int on_error_action_ = 0;
	QString custom_error_command_;
	int on_start_action_ = 0;

	// SpeedControlSettingPage(0 = 不限速)
	int max_overall_download_limit_ = 0;
	int max_overall_upload_limit_ = 0;
	int lowest_speed_limit_ = 0;

	// TimeoutRetrySettingPage(秒)
	int timeout_ = 60;
	int connect_timeout_ = 60;
	int max_tries_ = 5;
	int retry_wait_ = 0;

	// TrackerServerSettingPage(合法 JSON 空数组,避免 JSON.parse 抛异常)
	QString tracker_source_names_ = QStringLiteral("[]");
	bool enable_tracker_source_auto_update_ = false;

	// UserAgentSettingPage
	QString user_agent_ =
		QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
					   "Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0");

	// Ed2kSettingPage(默认值对齐 setting.h Default() 与 Ed2kSettingPage.qml 的 onReset)
	QString ed2k_nickname_ = QStringLiteral("GDownload");
	int ed2k_tcp_port_ = 4662;
	int ed2k_udp_port_ = 4672;
	bool ed2k_enable_kad_ = false;
	bool ed2k_enable_obfuscation_ = false;
	bool ed2k_auto_connect_ = true;
	int ed2k_max_concurrent_tasks_ = 5;
	QString ed2k_server_met_url_ = QStringLiteral("http://upd.emule-security.org/server.met");
	QString ed2k_nodes_dat_url_ = QStringLiteral("http://upd.emule-security.org/nodes.dat");
};

// ToastManager 桩:记录 ShowXxx 调用,供集成测试验证用户反馈。
class TestToastManager : public QObject {
	Q_OBJECT
   public:
	explicit TestToastManager(QObject* parent = nullptr) : QObject(parent) {}
	Q_INVOKABLE void ShowError(const QString& msg) { record(QStringLiteral("error"), msg); }
	Q_INVOKABLE void ShowSuccess(const QString& msg) { record(QStringLiteral("success"), msg); }
	Q_INVOKABLE void ShowWarning(const QString& msg) { record(QStringLiteral("warning"), msg); }
	Q_INVOKABLE void ShowInfo(const QString& msg) { record(QStringLiteral("info"), msg); }

	int messageCount() const { return messages_.size(); }
	QString lastType() const { return messages_.isEmpty() ? QString() : messages_.last().first; }
	QString lastMessage() const { return messages_.isEmpty() ? QString() : messages_.last().second; }
	void clearHistory() { messages_.clear(); }

   private:
	void record(const QString& type, const QString& message) {
		messages_.append(qMakePair(type, message));
	}

	QList<QPair<QString, QString>> messages_;
};

// 云目录模型桩:覆盖 NetDiskPageView 委托使用的全部角色，并允许运行时测试
// 注入大目录以验证 ListView 虚拟化和 resize 行为。
class TestNetDiskFileModel : public QAbstractListModel {
	Q_OBJECT
	Q_PROPERTY(int count READ count NOTIFY countChanged)
   public:
	enum Roles {
		kFileName = Qt::UserRole + 1,
		kFilePath,
		kFileId,
		kFileSize,
		kCreateTime,
		kIsDir,
		kIsSelected,
	};

	explicit TestNetDiskFileModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}
	int count() const { return rows_.size(); }

	int rowCount(const QModelIndex& parent = QModelIndex()) const override {
		if (parent.isValid()) return 0;
		return rows_.size();
	}

	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid() || index.row() < 0 || index.row() >= rows_.size()) return {};
		return rows_.at(index.row()).value(QString::fromUtf8(roleNames().value(role)));
	}

	QHash<int, QByteArray> roleNames() const override {
		return {{kFileName, "fileName"},
			{kFilePath, "filePath"},
			{kFileId, "fileId"},
			{kFileSize, "fileSize"},
			{kCreateTime, "createTime"},
			{kIsDir, "isDir"},
			{kIsSelected, "isSelected"}};
	}

	void setRows(const QList<QVariantMap>& rows) {
		beginResetModel();
		rows_ = rows;
		endResetModel();
		Q_EMIT countChanged();
	}

	void setSelected(int row, bool selected) {
		if (row < 0 || row >= rows_.size() || rows_[row].value("isSelected").toBool() == selected)
			return;
		rows_[row].insert("isSelected", selected);
		const QModelIndex changed = index(row, 0);
		Q_EMIT dataChanged(changed, changed, {kIsSelected});
	}

	void setAllSelected(bool selected) {
		if (rows_.isEmpty()) return;
		for (QVariantMap& row : rows_) row.insert("isSelected", selected);
		Q_EMIT dataChanged(index(0, 0), index(rows_.size() - 1, 0), {kIsSelected});
	}

   Q_SIGNALS:
	void countChanged();

   private:
	QList<QVariantMap> rows_;
};

// NetWorkDiskManager 桩:提供真实可绑定的目录模型和页面调用契约。
class TestNetWorkDiskManager : public QObject {
	Q_OBJECT
   public:
	explicit TestNetWorkDiskManager(QObject* parent = nullptr) : QObject(parent) {}
	Q_INVOKABLE void ParseShareUrl(const QString& url) { Q_UNUSED(url); }
	Q_INVOKABLE QObject* GetNetWorkDiskModel() { return &model_; }
	Q_INVOKABLE void ChangeDir(const QString& path, const QString& file_id) {
		Q_UNUSED(path);
		Q_UNUSED(file_id);
	}
	Q_INVOKABLE void ToggleSelection(int index, bool selected) { model_.setSelected(index, selected); }
	Q_INVOKABLE void SelectAll() { model_.setAllSelected(true); }
	Q_INVOKABLE void UnselectAll() { model_.setAllSelected(false); }

	void setRows(const QList<QVariantMap>& rows) { model_.setRows(rows); }
	TestNetDiskFileModel* model() { return &model_; }

   Q_SIGNALS:
	void taskFinished(const QString& message, bool success, int taskType);

   private:
	TestNetDiskFileModel model_{this};
};

// UpdateManager 桩:复刻 UpdateDialog 依赖的方法与信号契约
class TestUpdateManager : public QObject {
	Q_OBJECT
   public:
	explicit TestUpdateManager(QObject* parent = nullptr) : QObject(parent) {}
	Q_INVOKABLE bool StartUpdate() { return false; }

   Q_SIGNALS:
	void updateAvailable(const QVariantMap& info);
	void updateProgress(const QVariantMap& progress);
	void updateFinished(bool success);
};

// UtilsToolsManager 桩:HideMacOsxWindowStandardButtons 等空实现
// BasicSettingPage 调 SetAutoStart;Aria2RpcSettingPage 调 RelaunchAfterExit;
// TrackerServerSettingPage 绑定 serverList 文本。三者均提供桩,使页面加载与渲染不抛异常。
class TestUtilsToolsManager : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString serverList MEMBER server_list_ CONSTANT)
   public:
	explicit TestUtilsToolsManager(QObject* parent = nullptr) : QObject(parent) {}
	Q_INVOKABLE void HideMacOsxWindowStandardButtons(QObject* window) { Q_UNUSED(window); }
	Q_INVOKABLE bool SetClipboardText(const QString& text) {
		Q_UNUSED(text);
		return false;
	}
	Q_INVOKABLE QString Version() const { return QStringLiteral("test-stub"); }
	Q_INVOKABLE QString GetNoticeContent() const {
		return QStringLiteral("GDownload test notice\n\nThird-party license information is available in the packaged NOTICE file.");
	}
	// BasicSettingPage:开机自启动(空实现)
	Q_INVOKABLE void SetAutoStart(bool v) { Q_UNUSED(v); }
	// Aria2RpcSettingPage:保存后重启应用(空实现)
	Q_INVOKABLE void RelaunchAfterExit(int delayMs) { Q_UNUSED(delayMs); }

   private:
	// TrackerServerSettingPage TextArea 绑定此属性展示 tracker 列表
	QString server_list_ = QStringLiteral("udp://tracker.example.com:1337\nhttp://tracker.example.com/announce");
};

// ClipboardWatcher 桩:TaskDialogPage 在创建 URL 输入框时读取剪贴板,
// 并监听 clipboardChanged。视觉测试不触碰真实系统剪贴板,只提供稳定的空值契约。
class TestClipboardWatcher : public QObject {
	Q_OBJECT
	public:
	explicit TestClipboardWatcher(QObject* parent = nullptr) : QObject(parent) {}
	Q_INVOKABLE QString GetClipboardText() const { return QString(); }

	Q_SIGNALS:
	void clipboardChanged(const QString& text);
};

// LanguageManager 桩:BasicSettingPage 语言下拉引用 GetSupportedLanguages /
// GetCurrentLanguage / SwitchLanguage。返回固定列表与当前语言,使下拉能渲染选项。
class TestLanguageManager : public QObject {
	Q_OBJECT
   public:
	explicit TestLanguageManager(QObject* parent = nullptr) : QObject(parent) {}
	// BasicSettingPage 语言下拉 model 与 values 数据源
	Q_INVOKABLE QVariantList GetSupportedLanguages() const {
		return {QStringLiteral("en_US"), QStringLiteral("zh_CN"), QStringLiteral("zh_TW"),
				QStringLiteral("ja_JP"), QStringLiteral("ko_KR")};
	}
	Q_INVOKABLE QString GetCurrentLanguage() const { return QStringLiteral("en_US"); }
	Q_INVOKABLE void SwitchLanguage(const QString& lang) { Q_UNUSED(lang); }
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

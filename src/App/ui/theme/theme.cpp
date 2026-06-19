#include "theme.h"
#include <QApplication>
#include <QPalette>
#include <QQmlEngine>
#include "Settings/settings_manager.h"
#include "Definitions/elementPlusColors.h"
namespace gdl {

	namespace ui {

		namespace theme {
			GTheme* GTheme::create(QQmlEngine* qmlengine, QJSEngine* jsengine) {
				Q_UNUSED(qmlengine)
				Q_UNUSED(jsengine);
				return &GTheme::Instance();
			}

			bool GTheme::dark() const {
                // 简化 dark() 方法，仅返回主题状态，不处理样式表
				if (theme_ == GThemeType::ThemeMode::kDark) {
                    return true;
				}
				else if (theme_ == GThemeType::ThemeMode::kSystem) {
                    return system_is_dark_theme_;
				}
                return false;
			}

            // 新增方法：应用样式表（仅在主题切换时调用）
            void GTheme::applyMenuStyleSheet() {
                bool is_dark = dark();
                QString menu_style;
                if (is_dark) {
                    menu_style =
                        "QMenu{"
                        "background-color: " + QString(ElementPlusColors::VSCodeDark::kBgOverlay) + ";"
                        "border: 1px solid " + QString(ElementPlusColors::VSCodeDark::kBorderBase) + ";"
                        "border-radius: 4px;"
                        "padding: 4px 0;"
                        "}"
                        "QMenu::item{"
                        "background-color: transparent;"
                        "color: " + QString(ElementPlusColors::VSCodeDark::kTextPrimary) + ";"
                        "padding: 8px 16px;"
                        "margin: 0 4px;"
                        "border-radius: 4px;"
                        "}"
                        "QMenu::item:selected{"
                        "background-color: " + QString(ElementPlusColors::VSCodeDark::kFillLight) + ";"
                        "}"
                        "QMenu::item:hover{"
                        "background-color: " + QString(ElementPlusColors::VSCodeDark::kFillLight) + ";"
                        "}"
                        "QMenu::separator{"
                        "height: 1px;"
                        "background: " + QString(ElementPlusColors::VSCodeDark::kBorderLight) + ";"
                        "margin: 4px 12px;"
                        "}";
                }
                else {
                    menu_style =
                        "QMenu{"
                        "background-color: " + QString(ElementPlusColors::AntDesignLight::kBgWhite) + ";"
                        "border: 1px solid " + QString(ElementPlusColors::AntDesignLight::kBorderLight) + ";"
                        "border-radius: 4px;"
                        "padding: 4px 0;"
                        "}"
                        "QMenu::item{"
                        "background-color: transparent;"
                        "color: " + QString(ElementPlusColors::AntDesignLight::kTextPrimary) + ";"
                        "padding: 8px 16px;"
                        "margin: 0 4px;"
                        "border-radius: 4px;"
                        "}"
                        "QMenu::item:selected{"
                        "background-color: " + QString(ElementPlusColors::AntDesignLight::kFillLight) + ";"
                        "}"
                        "QMenu::item:hover{"
                        "background-color: " + QString(ElementPlusColors::AntDesignLight::kFillLight) + ";"
                        "}"
                        "QMenu::separator{"
                        "height: 1px;"
                        "background: " + QString(ElementPlusColors::AntDesignLight::kBorderLighter) + ";"
                        "margin: 4px 12px;"
                        "}";
                }
                qApp->setStyleSheet(menu_style);
			}

			GTheme::GTheme(QObject* parent) : QObject(parent), system_is_dark_theme_(SystemIsDarkTheme()) {
                QString theme_string = settings::Settings::Instance().GetTheme().toLower();
                if (theme_string == "system") {
                    Settheme(GThemeType::ThemeMode::kSystem);
                }
                else if (theme_string == "light") {
                    Settheme(GThemeType::ThemeMode::kLight);
                }
                else {
                    Settheme(GThemeType::ThemeMode::kDark);
                }
				qApp->installEventFilter(this);

                // 初始化时应用样式表
                applyMenuStyleSheet();

                connect(this, &GTheme::themeChanged, this, [this]() {
                    QString theme_string;
                    switch (theme_) {
                        case GThemeType::ThemeMode::kSystem:
                            theme_string = "system";
                            break;
                        case GThemeType::ThemeMode::kLight:
                            theme_string = "light";
                            break;
                        case GThemeType::ThemeMode::kDark:
                            theme_string = "dark";
                            break;
                    }
                    settings::Settings::Instance().SetTheme(theme_string);
                    // 主题切换时重新应用样式表
                    applyMenuStyleSheet();
                    Q_EMIT darkChanged();
                });
			}

			bool GTheme::SystemIsDarkTheme() const {
				QPalette palette = QApplication::palette();
				QColor color	 = palette.color(QPalette::Window);
				auto luminance	 = color.red() * 0.2126 + color.green() * 0.7152 + color.blue() * 0.0722;
				return luminance <= 255.0f / 2;
			}

			bool GTheme::eventFilter(QObject* obj, QEvent* event) {
				if (event->type() == QEvent::ApplicationPaletteChange) {
					system_is_dark_theme_ = SystemIsDarkTheme();
					// 系统主题变化时重新应用样式表
					applyMenuStyleSheet();
					Q_EMIT darkChanged();
					event->accept();
					return true;
				}
				return QObject::eventFilter(obj, event);
			}

			// Element Plus 颜色方法实现
			QColor GTheme::primaryColor() const {
				return QColor(ElementPlusColors::Primary::kPrimary);
			}

			QColor GTheme::successColor() const {
				return QColor(ElementPlusColors::Status::kSuccess);
			}

			QColor GTheme::warningColor() const {
				return QColor(ElementPlusColors::Status::kWarning);
			}

			QColor GTheme::dangerColor() const {
				return QColor(ElementPlusColors::Status::kDanger);
			}

			QColor GTheme::infoColor() const {
				return QColor(ElementPlusColors::Status::kInfo);
			}

			QColor GTheme::textPrimary() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kTextPrimary)
							  : QColor(ElementPlusColors::AntDesignLight::kTextPrimary);
			}

			QColor GTheme::textRegular() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kTextRegular)
							  : QColor(ElementPlusColors::AntDesignLight::kTextRegular);
			}

			QColor GTheme::textSecondary() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kTextSecondary)
							  : QColor(ElementPlusColors::AntDesignLight::kTextSecondary);
			}

			QColor GTheme::textPlaceholder() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kTextPlaceholder)
							  : QColor(ElementPlusColors::AntDesignLight::kTextPlaceholder);
			}

			QColor GTheme::textDisabled() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kTextDisabled)
							  : QColor(ElementPlusColors::AntDesignLight::kTextDisabled);
			}

			QColor GTheme::borderBase() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kBorderBase)
							  : QColor(ElementPlusColors::AntDesignLight::kBorderBase);
			}

			QColor GTheme::borderLight() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kBorderLight)
							  : QColor(ElementPlusColors::AntDesignLight::kBorderLight);
			}

			QColor GTheme::borderLighter() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kBorderLighter)
							  : QColor(ElementPlusColors::AntDesignLight::kBorderLighter);
			}

			QColor GTheme::fillBase() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kFillBase)
							  : QColor(ElementPlusColors::AntDesignLight::kFillBase);
			}

			QColor GTheme::fillLight() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kFillLight)
							  : QColor(ElementPlusColors::AntDesignLight::kFillLight);
			}

			QColor GTheme::fillLighter() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kFillLighter)
							  : QColor(ElementPlusColors::AntDesignLight::kFillLighter);
			}

			QColor GTheme::bgWhite() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kBgWhite)
							  : QColor(ElementPlusColors::AntDesignLight::kBgWhite);
			}

			QColor GTheme::bgPage() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kBgPage)
							  : QColor(ElementPlusColors::AntDesignLight::kBgPage);
			}

			QColor GTheme::bgBase() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kBgBase)
							  : QColor(ElementPlusColors::AntDesignLight::kBgBase);
			}

			QColor GTheme::bgElevated() const {
				return dark() ? QColor(ElementPlusColors::VSCodeDark::kBgElevated)
							  : QColor(ElementPlusColors::AntDesignLight::kBgWhite);
			}

			// 主色色阶:严格对齐 Element Plus light3/5/7/8/9 + dark2
			QColor GTheme::primaryLight(int level) const {
				switch (level) {
					case 3: return QColor(ElementPlusColors::Primary::kPrimaryLight3);
					case 5: return QColor(ElementPlusColors::Primary::kPrimaryLight5);
					case 7: return QColor(ElementPlusColors::Primary::kPrimaryLight7);
					case 8: return QColor(ElementPlusColors::Primary::kPrimaryLight8);
					case 9: return QColor(ElementPlusColors::Primary::kPrimaryLight9);
					default: return primaryColor();
				}
			}

			QColor GTheme::successLight(int level) const {
				switch (level) {
					case 3: return QColor(ElementPlusColors::Status::kSuccessLight3);
					case 5: return QColor(ElementPlusColors::Status::kSuccessLight5);
					case 7: return QColor(ElementPlusColors::Status::kSuccessLight7);
					case 8: return QColor(ElementPlusColors::Status::kSuccessLight8);
					case 9: return QColor(ElementPlusColors::Status::kSuccessLight9);
					default: return successColor();
				}
			}

			QColor GTheme::warningLight(int level) const {
				switch (level) {
					case 3: return QColor(ElementPlusColors::Status::kWarningLight3);
					case 5: return QColor(ElementPlusColors::Status::kWarningLight5);
					case 7: return QColor(ElementPlusColors::Status::kWarningLight7);
					case 8: return QColor(ElementPlusColors::Status::kWarningLight8);
					case 9: return QColor(ElementPlusColors::Status::kWarningLight9);
					default: return warningColor();
				}
			}

			QColor GTheme::dangerLight(int level) const {
				switch (level) {
					case 3: return QColor(ElementPlusColors::Status::kDangerLight3);
					case 5: return QColor(ElementPlusColors::Status::kDangerLight5);
					case 7: return QColor(ElementPlusColors::Status::kDangerLight7);
					case 8: return QColor(ElementPlusColors::Status::kDangerLight8);
					case 9: return QColor(ElementPlusColors::Status::kDangerLight9);
					default: return dangerColor();
				}
			}

			QColor GTheme::infoLight(int level) const {
				switch (level) {
					case 3: return QColor(ElementPlusColors::Status::kInfoLight3);
					case 5: return QColor(ElementPlusColors::Status::kInfoLight5);
					case 7: return QColor(ElementPlusColors::Status::kInfoLight7);
					case 8: return QColor(ElementPlusColors::Status::kInfoLight8);
					case 9: return QColor(ElementPlusColors::Status::kInfoLight9);
					default: return infoColor();
				}
			}

			// 阴影层级:对应 Element Plus box-shadow 四档,深浅色取色不同
			static QVariantMap makeElevation(const QColor& color, int blur, int offsetY, int spread = 0) {
				QVariantMap m;
				m.insert("color", color);
				m.insert("blur", blur);
				m.insert("offsetX", 0);
				m.insert("offsetY", offsetY);
				m.insert("spread", spread);
				return m;
			}

			QVariantMap GTheme::elevation1() const {
				// 轻微阴影:卡片默认
				QColor c = dark() ? QColor(0, 0, 0, 80) : QColor(0, 0, 0, 16);
				return makeElevation(c, 4, 1);
			}

			QVariantMap GTheme::elevation2() const {
				// 悬浮阴影:hover/floating
				QColor c = dark() ? QColor(0, 0, 0, 120) : QColor(0, 0, 0, 24);
				return makeElevation(c, 8, 2);
			}

			QVariantMap GTheme::elevation3() const {
				// 弹层阴影:dropdown/popover
				QColor c = dark() ? QColor(0, 0, 0, 150) : QColor(0, 0, 0, 32);
				return makeElevation(c, 12, 4);
			}

			QVariantMap GTheme::elevation4() const {
				// 对话框阴影:dialog
				QColor c = dark() ? QColor(0, 0, 0, 180) : QColor(0, 0, 0, 40);
				return makeElevation(c, 16, 8);
			}

			void RegisterTypes(QQmlEngine* engine) {
				qmlRegisterUncreatableMetaObject(GThemeType::staticMetaObject, GEXPORT_MODULE_URL, 1, 0, "GThemeType",
												 "theme type enum");
                qmlRegisterSingletonInstance<GTheme>(GEXPORT_MODULE_URL, 1, 0, "GTheme", &GTheme::Instance());

				// 注册Element Plus颜色
				ElementPlusColors::RegisterElementPlusColors(engine);
			}

		}  // namespace theme
	}  // namespace ui
}  // namespace gdl

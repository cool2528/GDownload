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
                bool is_dark = false;
				if (theme_ == GThemeType::ThemeMode::kDark) {
                    is_dark = true;
				}
				else if (theme_ == GThemeType::ThemeMode::kSystem) {
                    is_dark = system_is_dark_theme_;
				}

                // 使用Element Plus风格的菜单样式
                QString menu_style;
                if (is_dark) {
                    menu_style =
                        "QMenu{"
                        "background-color: " + QString(ElementPlusColors::DarkNeutral::kBgOverlay) + ";"
                        "border: 1px solid " + QString(ElementPlusColors::DarkNeutral::kBorderBase) + ";"
                        "border-radius: 4px;"
                        "padding: 4px 0;"
                        "}"
                        "QMenu::item{"
                        "background-color: transparent;"
                        "color: " + QString(ElementPlusColors::DarkNeutral::kTextPrimary) + ";"
                        "padding: 8px 16px;"
                        "margin: 0 4px;"
                        "border-radius: 4px;"
                        "}"
                        "QMenu::item:selected{"
                        "background-color: " + QString(ElementPlusColors::DarkNeutral::kFillLight) + ";"
                        "}"
                        "QMenu::item:hover{"
                        "background-color: " + QString(ElementPlusColors::DarkNeutral::kFillLight) + ";"
                        "}"
                        "QMenu::separator{"
                        "height: 1px;"
                        "background: " + QString(ElementPlusColors::DarkNeutral::kBorderLight) + ";"
                        "margin: 4px 12px;"
                        "}";
                }
                else {
                    menu_style =
                        "QMenu{"
                        "background-color: " + QString(ElementPlusColors::LightNeutral::kBgWhite) + ";"
                        "border: 1px solid " + QString(ElementPlusColors::LightNeutral::kBorderLight) + ";"
                        "border-radius: 4px;"
                        "padding: 4px 0;"
                        "}"
                        "QMenu::item{"
                        "background-color: transparent;"
                        "color: " + QString(ElementPlusColors::LightNeutral::kTextPrimary) + ";"
                        "padding: 8px 16px;"
                        "margin: 0 4px;"
                        "border-radius: 4px;"
                        "}"
                        "QMenu::item:selected{"
                        "background-color: " + QString(ElementPlusColors::LightNeutral::kFillLight) + ";"
                        "}"
                        "QMenu::item:hover{"
                        "background-color: " + QString(ElementPlusColors::LightNeutral::kFillLight) + ";"
                        "}"
                        "QMenu::separator{"
                        "height: 1px;"
                        "background: " + QString(ElementPlusColors::LightNeutral::kBorderLighter) + ";"
                        "margin: 4px 12px;"
                        "}";
                }
                qApp->setStyleSheet(menu_style);
                return is_dark;
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
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kTextPrimary)
							  : QColor(ElementPlusColors::LightNeutral::kTextPrimary);
			}

			QColor GTheme::textRegular() const {
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kTextRegular)
							  : QColor(ElementPlusColors::LightNeutral::kTextRegular);
			}

			QColor GTheme::textSecondary() const {
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kTextSecondary)
							  : QColor(ElementPlusColors::LightNeutral::kTextSecondary);
			}

			QColor GTheme::textPlaceholder() const {
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kTextPlaceholder)
							  : QColor(ElementPlusColors::LightNeutral::kTextPlaceholder);
			}

			QColor GTheme::textDisabled() const {
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kTextDisabled)
							  : QColor(ElementPlusColors::LightNeutral::kTextDisabled);
			}

			QColor GTheme::borderBase() const {
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kBorderBase)
							  : QColor(ElementPlusColors::LightNeutral::kBorderBase);
			}

			QColor GTheme::borderLight() const {
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kBorderLight)
							  : QColor(ElementPlusColors::LightNeutral::kBorderLight);
			}

			QColor GTheme::borderLighter() const {
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kBorderLighter)
							  : QColor(ElementPlusColors::LightNeutral::kBorderLighter);
			}

			QColor GTheme::fillBase() const {
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kFillBase)
							  : QColor(ElementPlusColors::LightNeutral::kFillBase);
			}

			QColor GTheme::fillLight() const {
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kFillLight)
							  : QColor(ElementPlusColors::LightNeutral::kFillLight);
			}

			QColor GTheme::fillLighter() const {
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kFillLighter)
							  : QColor(ElementPlusColors::LightNeutral::kFillLighter);
			}

			QColor GTheme::bgWhite() const {
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kBgWhite)
							  : QColor(ElementPlusColors::LightNeutral::kBgWhite);
			}

			QColor GTheme::bgPage() const {
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kBgPage)
							  : QColor(ElementPlusColors::LightNeutral::kBgPage);
			}

			QColor GTheme::bgBase() const {
				return dark() ? QColor(ElementPlusColors::DarkNeutral::kBgBase)
							  : QColor(ElementPlusColors::LightNeutral::kBgBase);
			}

			// 工具方法：获取颜色的不同层级
			QColor GTheme::primaryLight(int level) const {
				switch (level) {
					case 1: return QColor(ElementPlusColors::Primary::kPrimaryLight1);
					case 2: return QColor(ElementPlusColors::Primary::kPrimaryLight2);
					case 3: return QColor(ElementPlusColors::Primary::kPrimaryLight3);
					case 4: return QColor(ElementPlusColors::Primary::kPrimaryLight4);
					case 5: return QColor(ElementPlusColors::Primary::kPrimaryLight5);
					case 6: return QColor(ElementPlusColors::Primary::kPrimaryLight6);
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

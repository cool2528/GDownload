#include "theme.h"
#include <QApplication>
#include <QPalette>
#include <QQmlEngine>
#include "Settings/settings_manager.h"
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
                QString menu_style;
                if (is_dark) {
                    menu_style =
                        "QMenu{"
                        "background-color: #292a2d;"
                        "}"
                        "QMenu::item{"
                        "background-color: #292a2d;"
                        "color: #ffffff;"
                        "}"
                        "QMenu::item:selected{"
                        "background-color: #3f4042;"
                        "}"
                        "QMenu::item:hover{"
                        "background-color: #3f4042;"
                        "}"
                        "QMenu::separator{"
                        "height: 2px;"
                        "background:#35383b;"
                        "}";
                }
                else {
                    menu_style =
                        "QMenu{"
                        "background-color: white;"
                        "}"
                        "QMenu::item{"
                        "background-color: transparent;"
                        "color: #3b3b3b;"
                        "}"
                        "QMenu::item:selected{"
                        "background-color: #e8e8e9;"
                        "}"
                        "QMenu::item:hover{"
                        "background-color: #e8e8e9;"
                        "}"
                        "QMenu::separator{"
                        "height: 2px;"
                        "background:#dadce0;"
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

			void RegisterTypes(QQmlEngine* engine) {
				qmlRegisterUncreatableMetaObject(GThemeType::staticMetaObject, GEXPORT_MODULE_URL, 1, 0, "GThemeType",
												 "theme type enum");
                qmlRegisterSingletonInstance<GTheme>(GEXPORT_MODULE_URL, 1, 0, "GTheme", &GTheme::Instance());
			}

		}  // namespace theme
	}  // namespace ui
}  // namespace gdl

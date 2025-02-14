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
				if (theme_ == GThemeType::ThemeMode::kDark) {
					return true;
				}
				else if (theme_ == GThemeType::ThemeMode::kSystem) {
					return system_is_dark_theme_;
				}
				return false;
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

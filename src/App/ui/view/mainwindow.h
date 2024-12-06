#pragma once
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
namespace gd {
	namespace ui {
		class MainWindow : public QObject {
			Q_OBJECT
		   public:
			explicit MainWindow(QObject* parent = nullptr);
			~MainWindow();
			int Exec(int argc, char* argv[]);

		   private:
			void InitQmlEngine(QQmlEngine* engine);
			void InitTranslation(QGuiApplication* app);
			void InitFont(QQmlEngine* engine);
			void InitIcon(QGuiApplication* app);
			void UnInitEngine();

		   private:
			int fluent_icons_font_id_{-1};
		};
	}  // namespace ui
}  // namespace gd

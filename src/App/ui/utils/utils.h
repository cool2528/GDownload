/**
	 * @file utils.h
	 * @author cool
	 */
#include <QObject>
#include <QWindow>
#include "GDLCore/singleton.hpp"
class QQuickWindow;
class QQmlEngine;
namespace gdl {
	namespace ui {

		namespace utils {
#if defined(__APPLE__)
			void hideWindowStandardButtons(WId wid);
            void setTaskbarProgress(double progress);
#endif
			class UtilsToolsManager : public QObject, public Singleton<UtilsToolsManager> {
				Q_OBJECT
				SINGLETON_DECLARE(UtilsToolsManager)
			   public:
				~UtilsToolsManager() override;
#if defined(__APPLE__)
				Q_INVOKABLE void HideMacOsxWindowStandardButtons(QQuickWindow* window);
#endif
                Q_INVOKABLE bool SetClipboardText(const QString& text);

                Q_INVOKABLE void SetTaskbarProgress(double progress, void* nativeWindowHandle = nullptr);

			   private:
				explicit UtilsToolsManager(QObject* parent = nullptr);
			};
			void RegisterTypes(QQmlEngine* engine);
		}  // namespace utils
	}  // namespace ui
}  // namespace gdl

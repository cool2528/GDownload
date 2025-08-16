/**
	 * @file utils.h
	 * @author cool
	 */
#include <QObject>
#include <QWindow>
#include "GDLCore/singleton.hpp"
#include "Definitions/autoProperty.h"
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
				QML_AUTO_PROPERTY_V(QString, serverList, "")
			   public:
				~UtilsToolsManager() override;
#if defined(__APPLE__)
				Q_INVOKABLE void HideMacOsxWindowStandardButtons(QQuickWindow* window);
#endif
                Q_INVOKABLE bool SetClipboardText(const QString& text);

                Q_INVOKABLE void SetTaskbarProgress(double progress, void* nativeWindowHandle = nullptr);
                Q_INVOKABLE QString Version() const;
                Q_INVOKABLE QString GetNoticeContent() const;

                // 设置开机自启动
                Q_INVOKABLE bool SetAutoStart(bool enable);
                
                // 获取当前自启动状态
                Q_INVOKABLE bool IsAutoStartEnabled() const;

			   private:
				explicit UtilsToolsManager(QObject* parent = nullptr);
				
				// 内部平台相关的自启动实现
				bool SetAutoStartImpl(bool enable);
				bool IsAutoStartEnabledImpl() const;
			};
			void RegisterTypes(QQmlEngine* engine);
		}  // namespace utils
	}  // namespace ui
}  // namespace gdl

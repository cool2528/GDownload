#pragma once
#include <QObject>
#include <QVariantList>
#include <QtQml/qqml.h>

namespace gd {
	namespace ui {
		// 浏览器扩展安装引导管理器（QML 单例）
		// - extensionPaired：Native host 是否收到过扩展握手（设计文档完成判据）
		// - detectedBrowsers：本机已安装的浏览器清单（供引导 UI 按浏览器定制）
		class InstallGuideManager : public QObject {
			Q_OBJECT
			Q_PROPERTY(bool extensionPaired READ extensionPaired NOTIFY pairedChanged)
		   public:
			static InstallGuideManager& Instance();

			bool extensionPaired() const { return paired_; }

			// 重新检测配对状态（读握手标记文件），状态变化则发信号
			Q_INVOKABLE void refresh();

			// 返回已安装浏览器：[{ id, name, installed }]
			Q_INVOKABLE QVariantList detectedBrowsers() const;

		   signals:
			void pairedChanged();

		   private:
			explicit InstallGuideManager(QObject* parent = nullptr);
			static QString MarkerPath();

			bool paired_ = false;
		};
	}  // namespace ui
}  // namespace gd

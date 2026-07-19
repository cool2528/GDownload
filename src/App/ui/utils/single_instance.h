#pragma once
#include <QObject>
#include <QString>
#include <QStringList>

class QLocalServer;

namespace gd {
	namespace ui {
		// 单实例守卫：首个实例成为主实例并监听；后续实例把参数转发给主实例后退出。
		// 基于 QLocalServer/QLocalSocket（跨平台，Windows 走命名管道）。
		class SingleInstanceGuard : public QObject {
			Q_OBJECT
		   public:
			explicit SingleInstanceGuard(const QString& key, QObject* parent = nullptr);
			~SingleInstanceGuard() override;

			// 尝试成为主实例。成功返回 true（已开始监听后续实例）；
			// 已有实例在运行返回 false。
			bool tryBecomePrimary();

			// 作为次实例，把参数发送给主实例（在 tryBecomePrimary 返回 false 后调用）。
			bool sendToPrimary(const QStringList& args);

		   signals:
			// 主实例收到次实例转发的参数
			void messageReceived(const QStringList& args);

		   private:
			void onNewConnection();

			QString server_name_;
			QLocalServer* server_ = nullptr;
		};
	}  // namespace ui
}  // namespace gd

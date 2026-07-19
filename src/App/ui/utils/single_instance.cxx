#include "single_instance.h"

#include <QDataStream>
#include <QLocalServer>
#include <QLocalSocket>

#include "GDLCore/logger.h"

namespace gd {
	namespace ui {

		namespace {
			constexpr int kConnectTimeoutMs = 500;
			constexpr int kReadWriteTimeoutMs = 1000;
		}  // namespace

		SingleInstanceGuard::SingleInstanceGuard(const QString& key, QObject* parent)
			: QObject(parent), server_name_(key) {}

		SingleInstanceGuard::~SingleInstanceGuard() {
			if (server_) {
				server_->close();
			}
		}

		bool SingleInstanceGuard::tryBecomePrimary() {
			// 先探测是否已有主实例：能连上说明已有实例
			{
				QLocalSocket probe;
				probe.connectToServer(server_name_);
				if (probe.waitForConnected(kConnectTimeoutMs)) {
					probe.disconnectFromServer();
					return false;
				}
			}

			// 清理可能残留的服务端点（上次异常退出未清理）后开始监听
			QLocalServer::removeServer(server_name_);
			server_ = new QLocalServer(this);
			if (!server_->listen(server_name_)) {
				LOG_ERR("SingleInstanceGuard listen failed: {}", server_->errorString().toStdString());
				// 监听失败时保守按主实例处理（不阻塞启动）
				return true;
			}
			connect(server_, &QLocalServer::newConnection, this, &SingleInstanceGuard::onNewConnection);
			return true;
		}

		bool SingleInstanceGuard::sendToPrimary(const QStringList& args) {
			QLocalSocket socket;
			socket.connectToServer(server_name_);
			if (!socket.waitForConnected(kConnectTimeoutMs)) {
				return false;
			}
			QByteArray payload;
			{
				QDataStream stream(&payload, QIODevice::WriteOnly);
				stream.setVersion(QDataStream::Qt_6_0);
				stream << args;
			}
			socket.write(payload);
			socket.flush();
			const bool written = socket.waitForBytesWritten(kReadWriteTimeoutMs);
			socket.disconnectFromServer();
			if (socket.state() != QLocalSocket::UnconnectedState) {
				socket.waitForDisconnected(kReadWriteTimeoutMs);
			}
			return written;
		}

		void SingleInstanceGuard::onNewConnection() {
			while (QLocalSocket* socket = server_ ? server_->nextPendingConnection() : nullptr) {
				// 小数据量，直接同步读取全部
				if (!socket->waitForReadyRead(kReadWriteTimeoutMs)) {
					socket->deleteLater();
					continue;
				}
				QByteArray payload = socket->readAll();
				while (socket->waitForReadyRead(50)) {
					payload += socket->readAll();
				}
				QStringList args;
				{
					QDataStream stream(&payload, QIODevice::ReadOnly);
					stream.setVersion(QDataStream::Qt_6_0);
					stream >> args;
				}
				socket->deleteLater();
				emit messageReceived(args);
			}
		}

	}  // namespace ui
}  // namespace gd

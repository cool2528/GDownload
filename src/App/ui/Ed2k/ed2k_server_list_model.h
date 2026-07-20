#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace gdl {
	namespace ui {
		namespace ed2k {

			// 服务器列表行
			struct Ed2kServerItem {
				QString name;
				QString ip;
				quint16 port = 0;
				quint32 users = 0;
				quint32 files = 0;
				quint32 max_users = 0;
				bool connected = false;
			};

			// eD2k 服务器列表模型：connected 行置顶，其余按用户数降序
			class Ed2kServerListModel : public QAbstractListModel {
				Q_OBJECT
				Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
			   public:
				enum Roles {
					kName = Qt::UserRole + 1,
					kIp,
					kPort,
					kAddress,
					kUsers,
					kFiles,
					kMaxUsers,
					kConnected,
				};
				Q_ENUM(Roles)

				explicit Ed2kServerListModel(QObject* parent = nullptr);

				int rowCount(const QModelIndex& parent = QModelIndex()) const override;
				QVariant data(const QModelIndex& index, int role) const override;
				QHash<int, QByteArray> roleNames() const override;

				// 整批替换，内部按 connected 置顶、其余按 users 降序排列
				void ResetItems(const QVector<Ed2kServerItem>& items);

			   Q_SIGNALS:
				void countChanged();

			   private:
				QVector<Ed2kServerItem> items_;
			};

		}  // namespace ed2k
	}  // namespace ui
}  // namespace gdl

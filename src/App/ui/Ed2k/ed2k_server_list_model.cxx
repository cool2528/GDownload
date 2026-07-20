#include "ed2k_server_list_model.h"

#include <algorithm>

namespace gdl {
	namespace ui {
		namespace ed2k {

			Ed2kServerListModel::Ed2kServerListModel(QObject* parent) : QAbstractListModel(parent) {}

			int Ed2kServerListModel::rowCount(const QModelIndex& parent) const {
				if (parent.isValid()) return 0;
				return items_.size();
			}

			QVariant Ed2kServerListModel::data(const QModelIndex& index, int role) const {
				if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) return {};
				const auto& item = items_.at(index.row());
				switch (role) {
					case kName: return item.name;
					case kIp: return item.ip;
					case kPort: return item.port;
					case kAddress: return QStringLiteral("%1:%2").arg(item.ip).arg(item.port);
					case kUsers: return item.users;
					case kFiles: return item.files;
					case kMaxUsers: return item.max_users;
					case kConnected: return item.connected;
					default: return {};
				}
			}

			QHash<int, QByteArray> Ed2kServerListModel::roleNames() const {
				return {
					{kName, "serverName"}, {kIp, "serverIp"},     {kPort, "serverPort"},
					{kAddress, "serverAddress"}, {kUsers, "users"}, {kFiles, "files"},
					{kMaxUsers, "maxUsers"},     {kConnected, "connected"},
				};
			}

			void Ed2kServerListModel::ResetItems(const QVector<Ed2kServerItem>& items) {
				beginResetModel();
				items_ = items;
				std::stable_sort(items_.begin(), items_.end(),
								 [](const Ed2kServerItem& a, const Ed2kServerItem& b) {
									 if (a.connected != b.connected) return a.connected;
									 return a.users > b.users;
								 });
				endResetModel();
				Q_EMIT countChanged();
			}

		}  // namespace ed2k
	}  // namespace ui
}  // namespace gdl

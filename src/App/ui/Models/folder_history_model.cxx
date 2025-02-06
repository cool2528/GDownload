#include "folder_history_model.h"
#include <QSettings>

namespace gdl {
    namespace ui {
        namespace models {
            FolderHistoryModel::FolderHistoryModel(QObject* parent) : QAbstractListModel(parent) {
                loadHistory();
            }

            int FolderHistoryModel::rowCount(const QModelIndex& parent) const {
                if (parent.isValid()) return 0;
                return history_paths_.count();
            }

            QVariant FolderHistoryModel::data(const QModelIndex& index, int role) const {
                if (!index.isValid() || index.row() >= history_paths_.count()) return QVariant();

                if (role == PathRole) return history_paths_.at(index.row());

                return QVariant();
            }

            QHash<int, QByteArray> FolderHistoryModel::roleNames() const {
                QHash<int, QByteArray> roles;
                roles[PathRole] = "path";
                return roles;
            }

            void FolderHistoryModel::addPath(const QString& path) {
                // 移除已存在的相同路径
                history_paths_.removeAll(path);

                // 在开头插入新路径
                beginInsertRows(QModelIndex(), 0, 0);
                history_paths_.prepend(path);
                endInsertRows();

                // 确保不超过最大数量
                while (history_paths_.size() > max_history_count_) {
                    beginRemoveRows(QModelIndex(), history_paths_.size() - 1, history_paths_.size() - 1);
                    history_paths_.removeLast();
                    endRemoveRows();
                }

                saveHistory();
            }

            void FolderHistoryModel::removePath(int index) {
                if (index < 0 || index >= history_paths_.count()) return;

                beginRemoveRows(QModelIndex(), index, index);
                history_paths_.removeAt(index);
                endRemoveRows();

                saveHistory();
            }

            void FolderHistoryModel::clear() {
                beginResetModel();
                history_paths_.clear();
                endResetModel();

                saveHistory();
            }

            void FolderHistoryModel::setMaxHistoryCount(int count) {
                if (max_history_count_ == count) return;

                max_history_count_ = count;

                // 如果当前历史记录超过新的最大值，则删除多余项
                while (history_paths_.size() > max_history_count_) {
                    beginRemoveRows(QModelIndex(), history_paths_.size() - 1, history_paths_.size() - 1);
                    history_paths_.removeLast();
                    endRemoveRows();
                }

                emit maxHistoryCountChanged();
                saveHistory();
            }

            void FolderHistoryModel::loadHistory() {
                QSettings settings;
                history_paths_ = settings.value("FolderHistory/paths").toStringList();

                // 确保不超过最大数量
                while (history_paths_.size() > max_history_count_) {
                    history_paths_.removeLast();
                }
            }

            void FolderHistoryModel::saveHistory() {
                QSettings settings;
                settings.setValue("FolderHistory/paths", history_paths_);
            }
        }  // namespace models
    }  // namespace ui
}  // namespace gdl

#include "NetWork_disk_model.h"
#include <QDateTime>
#include <QJSEngine>
namespace gdl {
    namespace ui {

        NetWorkDiskModel::NetWorkDiskModel(QObject* parent) : QAbstractListModel(parent) {
            QJSEngine::setObjectOwnership(this, QJSEngine::CppOwnership);
        }

        NetWorkDiskModel::~NetWorkDiskModel() {}

        int NetWorkDiskModel::rowCount(const QModelIndex& parent) const {
            if (parent.isValid()) return 0;  // 扁平列表，父项有效时无子项
            return files_.size();
        }

        QVariant NetWorkDiskModel::data(const QModelIndex& index, int role) const {
            if (!index.isValid()) return QVariant();
            auto row = index.row();
            if (row >= files_.size()) return QVariant();
            const auto& file = files_[row];
            switch (role) {
                case FileNameRole:
                    return file.file_name;
                case FileSizeRole:
                    return file.file_size;
                case FilePathRole:
                    return file.file_path;
                case FileIdRole:
                    return file.file_id;
                case CreateTimeRole:
                    return file.create_time;
                case IsDirRole:
                    return file.is_dir;
                case DownloadUrlRole:
                    return file.download_url;
                case IsSelectedRole:
                    return file.is_selected;
                default:
                    break;
            }
            return QVariant();
        }

        bool NetWorkDiskModel::setData(const QModelIndex& index, const QVariant& value, int role) {
            if (!index.isValid()) return false;
            auto row = index.row();
            if (row >= files_.size()) return false;
            switch (role) {
                case IsDirRole:
                    files_[row].is_dir = value.toBool();
                    break;
                case IsSelectedRole:
                    files_[row].is_selected = value.toBool();
                    break;
                default:
                    return false;
            }
            Q_EMIT dataChanged(index, index, {role});
            return true;
        }

        QHash<int, QByteArray> NetWorkDiskModel::roleNames() const {
            QHash<int, QByteArray> roles;
            roles[FileNameRole]	   = "fileName";
            roles[FileSizeRole]	   = "fileSize";
            roles[FilePathRole]	   = "filePath";
            roles[FileIdRole]	   = "fileId";
            roles[CreateTimeRole]  = "createTime";
            roles[IsDirRole]	   = "isDir";
            roles[DownloadUrlRole] = "downloadUrl";
            roles[IsSelectedRole]  = "isSelected";
            return roles;
        }

        void NetWorkDiskModel::SelectAll() {
            if (files_.isEmpty()) return;  // 空列表时 index(-1) 为无效索引，避免违反模型契约
            for (auto& file : files_) {
                file.is_selected = true;
            }
            Q_EMIT dataChanged(index(0), index(files_.size() - 1), {IsSelectedRole});
        }

        void NetWorkDiskModel::UnselectAll() {
            if (files_.isEmpty()) return;
            for (auto& file : files_) {
                file.is_selected = false;
            }
            Q_EMIT dataChanged(index(0), index(files_.size() - 1), {IsSelectedRole});
        }

        bool NetWorkDiskModel::Init(const std::vector<INetDiskDownloadPlugin::FileInfo>& files) {
            beginResetModel();
            files_.clear();
            for (const auto& file : files) {
                NetWorkDiskData data;
                data.file_name	 = QString::fromStdString(file.name);
                data.file_size	 = FileSizeToString(file.size);
                data.file_path	 = QString::fromStdString(file.path);
                data.file_id	 = QString::fromStdString(file.file_id);
                data.create_time = CreateTimeToString(file.create_time);
                data.is_dir		 = file.is_dir;
                files_.push_back(data);
            }
            endResetModel();
            Q_EMIT sigLoadFinished();
            return true;
        }

        void NetWorkDiskModel::ToggleSelection(int index, bool is_selected) {
            if (index >= 0 && index < files_.size()) {
                files_[index].is_selected = is_selected;
                emit dataChanged(this->index(index), this->index(index), {IsSelectedRole});
            }
        }

        QVector<NetWorkDiskData> NetWorkDiskModel::GetSelectedFiles() {
            QVector<NetWorkDiskData> selected_files;
            for (const auto& file : files_) {
                if (file.is_selected) {
                    selected_files.push_back(file);
                }
            }
            return selected_files;
        }

        QString NetWorkDiskModel::FileSizeToString(quint64 size) {
            if (size < 1024) {
                return QString::number(size) + " B";
            }
            else if (size < 1024 * 1024) {
                return QString::number(size / 1024) + " KB";
            }
            else if (size < 1024 * 1024 * 1024) {
                return QString::number(size / 1024 / 1024) + " MB";
            }
            else {
                return QString::number(size / 1024 / 1024 / 1024) + " GB";
            }
            return QString();
        }

        QString NetWorkDiskModel::CreateTimeToString(quint64 time) {
            return QDateTime::fromSecsSinceEpoch(time).toString("yyyy-MM-dd hh:mm:ss");
        }

    }  // namespace ui
}  // namespace gdl

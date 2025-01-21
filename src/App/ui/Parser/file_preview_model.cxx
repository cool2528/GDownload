#include "file_preview_model.h"
#include <QFileInfo>
#include <numeric>

namespace gdl {
    namespace ui {
        namespace parser {

            QString PreviewFileInfo::FormatFileSize(qint64 size) {
                constexpr qint64 kb = 1024;
                constexpr qint64 mb = kb * 1024;
                constexpr qint64 gb = mb * 1024;
                constexpr qint64 tb = gb * 1024;

                if (size >= tb) {
                    return QString::number(size / tb, 'f', 2) + " TB";
                }
                else if (size >= gb) {
                    return QString::number(size / gb, 'f', 2) + " GB";
                }
                else if (size >= mb) {
                    return QString::number(size / mb, 'f', 2) + " MB";
                }
                else if (size >= kb) {
                    return QString::number(size / kb, 'f', 2) + " KB";
                }
                return QString::number(size) + " B";
            }

            FilePreviewModel::FilePreviewModel(QObject* parent) : QAbstractListModel(parent) {}

            int FilePreviewModel::rowCount(const QModelIndex& parent) const {
                if (parent.isValid()) return 0;
                return files_.size();
            }

            QVariant FilePreviewModel::data(const QModelIndex& index, int role) const {
                if (!index.isValid() || index.row() >= files_.size()) return QVariant();

                const auto& file = files_[index.row()];
                switch (role) {
                    case FileNameRole:
                        return file.file_name;
                    case ExtensionRole:
                        return file.file_extension;
                    case FileSizeRole:
                        return file.file_size;
                    case IsSelectedRole:
                        return file.is_selected;
                    default:
                        return QVariant();
                }
            }

            bool FilePreviewModel::setData(const QModelIndex& index, const QVariant& value, int role) {
                if (!index.isValid() || index.row() >= files_.size()) return false;

                if (role == IsSelectedRole) {
                    files_[index.row()].is_selected = value.toBool();
                    Q_EMIT dataChanged(index, index, {role});
                    updateTotalSize();
                    return true;
                }
                return false;
            }

            QHash<int, QByteArray> FilePreviewModel::roleNames() const {
                return {{FileNameRole, "fileName"},
                        {ExtensionRole, "fileExtension"},
                        {FileSizeRole, "fileSize"},
                        {IsSelectedRole, "isSelected"}};
            }

            void FilePreviewModel::selectAll() {
                for (auto& file : files_) {
                    file.is_selected = true;
                }
                Q_EMIT dataChanged(index(0), index(files_.size() - 1), {IsSelectedRole});
                updateTotalSize();
            }

            void FilePreviewModel::unselectAll() {
                for (auto& file : files_) {
                    file.is_selected = false;
                }
                Q_EMIT dataChanged(index(0), index(files_.size() - 1), {IsSelectedRole});
                updateTotalSize();
            }

            void FilePreviewModel::toggleSelection(int index) {
                if (index >= 0 && index < files_.size()) {
                    files_[index].is_selected = !files_[index].is_selected;
                    emit dataChanged(this->index(index), this->index(index), {IsSelectedRole});
                    updateTotalSize();
                }
            }

            QStringList FilePreviewModel::getSelectedFiles() const {
                QStringList result;
                for (const auto& file : files_) {
                    if (file.is_selected) {
                        result.append(file.file_name);
                    }
                }
                return result;
            }

            void FilePreviewModel::setFiles(const QVector<PreviewFileInfo>& files) {
                beginResetModel();
                files_ = files;
                endResetModel();
                updateTotalSize();
            }

            int FilePreviewModel::selectedCount() const {
                return std::count_if(files_.begin(), files_.end(),
                                     [](const PreviewFileInfo& file) { return file.is_selected; });
            }

            QString FilePreviewModel::totalSize() const {
                qint64 total = 0;
                for (const auto& file : files_) {
                    if (file.is_selected) {
                        // 这里假设file_size是字符串形式的大小,需要解析
                        QStringList parts = file.file_size.split(" ");
                        if (parts.size() == 2) {
                            double size	 = parts[0].toDouble();
                            QString unit = parts[1];
                            if (unit == "TB") size *= 1024 * 1024 * 1024 * 1024;
                            else if (unit == "GB") size *= 1024 * 1024 * 1024;
                            else if (unit == "MB") size *= 1024 * 1024;
                            else if (unit == "KB") size *= 1024;
                            total += static_cast<qint64>(size);
                        }
                    }
                }
                return PreviewFileInfo::FormatFileSize(total);
            }

            void FilePreviewModel::updateTotalSize() {
                int count	 = selectedCount();
                QString size = totalSize();
                Q_EMIT selectedCountChanged();
                Q_EMIT totalSizeChanged();
            }

        }  // namespace parser
    }  // namespace ui
}  // namespace gdl

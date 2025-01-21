#pragma once
#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVector>

namespace gdl {
    namespace ui {
        namespace parser {

            struct PreviewFileInfo {
                QString file_name;
                QString file_extension;
                QString file_size;
                bool is_selected{true};

                static QString FormatFileSize(qint64 size);
            };

            class FilePreviewModel : public QAbstractListModel {
                Q_OBJECT
                Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
                Q_PROPERTY(QString totalSize READ totalSize NOTIFY totalSizeChanged)

               public:
                enum Roles { FileNameRole = Qt::UserRole + 1, ExtensionRole, FileSizeRole, IsSelectedRole };

                explicit FilePreviewModel(QObject* parent = nullptr);

                int rowCount(const QModelIndex& parent = QModelIndex()) const override;
                QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
                bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
                QHash<int, QByteArray> roleNames() const override;

                Q_INVOKABLE void selectAll();
                Q_INVOKABLE void unselectAll();
                Q_INVOKABLE void toggleSelection(int index);
                Q_INVOKABLE QStringList getSelectedFiles() const;

                void setFiles(const QVector<PreviewFileInfo>& files);
                int selectedCount() const;
                QString totalSize() const;

                Q_INVOKABLE void clear() {
                    beginResetModel();
                    files_.clear();
                    endResetModel();
                    updateTotalSize();
                }

               signals:
                void selectedCountChanged();
                void totalSizeChanged();

               private:
                QVector<PreviewFileInfo> files_;
                void updateTotalSize();
            };

        }  // namespace parser
    }  // namespace ui
}  // namespace gdl

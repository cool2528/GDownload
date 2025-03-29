#pragma once
#include <QAbstractListModel>
#include <unordered_map>
#include "PluginManager/IDownload_Plugin.h"
namespace gdl {
    namespace ui {
        struct NetWorkDiskData {
            QString file_name;
            QString file_size;
            QString file_path;
            QString file_id;
            bool is_selected{false};
            QString create_time;
            bool is_dir{false};
            QString download_url;
            std::unordered_multimap<std::string, std::string> headers;
        };
        class NetWorkDiskModel : public QAbstractListModel {
            Q_OBJECT
            enum Roles {
                FileNameRole = Qt::UserRole + 1,
                FileSizeRole,
                FilePathRole,
                FileIdRole,
                CreateTimeRole,
                IsDirRole,
                DownloadUrlRole,
                IsSelectedRole
            };

           public:
            explicit NetWorkDiskModel(QObject* parent = nullptr);
            ~NetWorkDiskModel() override;
            int rowCount(const QModelIndex& parent = QModelIndex()) const override;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
            bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
            QHash<int, QByteArray> roleNames() const override;
            void SelectAll();
            void UnselectAll();
            bool Init(const std::vector<INetDiskDownloadPlugin::FileInfo>& files);
            void ToggleSelection(int index, bool is_selected);
            QVector<NetWorkDiskData> GetSelectedFiles();

           public:
           Q_SIGNALS:
            void sigLoadFinished();

           private:
            static QString FileSizeToString(quint64 size);
            static QString CreateTimeToString(quint64 time);

           private:
            QVector<NetWorkDiskData> files_;
        };
    }  // namespace ui
}  // namespace gdl

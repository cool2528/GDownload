#pragma once

#include <QAbstractListModel>
#include <QStringList>

namespace gdl {
    namespace ui {
        namespace models {
            class FolderHistoryModel : public QAbstractListModel {
                Q_OBJECT

                // 用于QML访问的属性
                Q_PROPERTY(
                    int maxHistoryCount READ maxHistoryCount WRITE setMaxHistoryCount NOTIFY maxHistoryCountChanged)

               public:
                explicit FolderHistoryModel(QObject* parent = nullptr);

                // QAbstractListModel必需的重写函数
                int rowCount(const QModelIndex& parent = QModelIndex()) const override;
                QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
                QHash<int, QByteArray> roleNames() const override;

                // 历史记录管理方法
                Q_INVOKABLE void addPath(const QString& path);
                Q_INVOKABLE void removePath(int index);
                Q_INVOKABLE void clear();

                // 属性访问器
                int maxHistoryCount() const { return max_history_count_; }
                void setMaxHistoryCount(int count);

               signals:
                void maxHistoryCountChanged();

               private:
                void loadHistory();
                void saveHistory();

                QStringList history_paths_;
                int max_history_count_ = 10;

                // 角色枚举
                enum Roles { PathRole = Qt::UserRole + 1 };
            };
        }  // namespace models
    }  // namespace ui
}  // namespace gdl

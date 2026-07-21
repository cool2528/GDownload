#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace gdl {
	namespace ui {
		namespace ed2k {

			// 单条本地共享文件（自身作为源提供给他人下载的文件）
			struct Ed2kSharedItem {
				QString name;
				QString path;
				qint64 size = 0;
				QString hash_hex;  // 32 位大写十六进制 md4
				qint64 uploaded = 0;
				quint32 requests = 0;
			};

			// eD2k 共享文件列表模型：Reset 按请求次数降序排列
			class Ed2kSharedFileModel : public QAbstractListModel {
				Q_OBJECT
				Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
			   public:
				enum Roles {
					kName = Qt::UserRole + 1,
					kPath,
					kSize,
					kSizeText,
					kUploaded,
					kUploadedText,
					kRequests,
					kRawLink,
				};
				Q_ENUM(Roles)

				explicit Ed2kSharedFileModel(QObject* parent = nullptr);

				int rowCount(const QModelIndex& parent = QModelIndex()) const override;
				QVariant data(const QModelIndex& index, int role) const override;
				QHash<int, QByteArray> roleNames() const override;

				// 整批替换，内部按 requests 降序排列
				void ResetItems(const QVector<Ed2kSharedItem>& items);

			   Q_SIGNALS:
				void countChanged();

			   private:
				QVector<Ed2kSharedItem> items_;
			};

		}  // namespace ed2k
	}  // namespace ui
}  // namespace gdl

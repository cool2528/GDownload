#pragma once

#include <QAbstractListModel>
#include <QVector>

namespace gdl {
	namespace ui {
		namespace ed2k {

			// 单条搜索结果（服务器/Kad 搜索统一结构）
			struct Ed2kSearchItem {
				QString name;
				qint64 size = 0;
				QString hash_hex;  // 32 位大写十六进制 md4
				quint32 sources = 0;
				quint32 complete_sources = 0;
			};

			// eD2k 搜索结果列表模型：Reset 按源数降序，Append(翻页)按 hash 去重
			class Ed2kSearchResultModel : public QAbstractListModel {
				Q_OBJECT
				Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
			   public:
				enum Roles {
					kName = Qt::UserRole + 1,
					kSize,
					kSizeText,
					kHash,
					kSources,
					kCompleteSources,
					kRawLink,
				};
				Q_ENUM(Roles)

				explicit Ed2kSearchResultModel(QObject* parent = nullptr);

				int rowCount(const QModelIndex& parent = QModelIndex()) const override;
				QVariant data(const QModelIndex& index, int role) const override;
				QHash<int, QByteArray> roleNames() const override;

				// 整批替换（首次搜索结果），内部按 sources 降序排列
				void ResetItems(const QVector<Ed2kSearchItem>& items);
				// 追加批次（Load More），按 hash 去重
				void AppendItems(const QVector<Ed2kSearchItem>& items);
				Q_INVOKABLE void clear();

			   Q_SIGNALS:
				void countChanged();

			   private:
				QVector<Ed2kSearchItem> items_;
			};

		}  // namespace ed2k
	}  // namespace ui
}  // namespace gdl

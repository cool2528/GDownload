#include "ed2k_search_result_model.h"

#include <algorithm>

#include <QLocale>
#include <QSet>

#include "ed2k_link_builder.h"

namespace gdl {
	namespace ui {
		namespace ed2k {

			Ed2kSearchResultModel::Ed2kSearchResultModel(QObject* parent) : QAbstractListModel(parent) {}

			int Ed2kSearchResultModel::rowCount(const QModelIndex& parent) const {
				if (parent.isValid()) return 0;
				return items_.size();
			}

			QVariant Ed2kSearchResultModel::data(const QModelIndex& index, int role) const {
				if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) return {};
				const auto& item = items_.at(index.row());
				switch (role) {
					case kName: return item.name;
					case kSize: return item.size;
					case kSizeText: return QLocale().formattedDataSize(item.size);
					case kHash: return item.hash_hex;
					case kSources: return item.sources;
					case kCompleteSources: return item.complete_sources;
					case kRawLink: return BuildRawLink(item.name, item.size, item.hash_hex);
					default: return {};
				}
			}

			QHash<int, QByteArray> Ed2kSearchResultModel::roleNames() const {
				return {
					{kName, "fileName"},          {kSize, "fileSize"},
					{kSizeText, "fileSizeText"},  {kHash, "fileHash"},
					{kSources, "sources"},        {kCompleteSources, "completeSources"},
					{kRawLink, "rawLink"},
				};
			}

			void Ed2kSearchResultModel::ResetItems(const QVector<Ed2kSearchItem>& items) {
				beginResetModel();
				items_ = items;
				std::stable_sort(items_.begin(), items_.end(),
								 [](const Ed2kSearchItem& a, const Ed2kSearchItem& b) { return a.sources > b.sources; });
				endResetModel();
				Q_EMIT countChanged();
			}

			void Ed2kSearchResultModel::AppendItems(const QVector<Ed2kSearchItem>& items) {
				QSet<QString> seen;
				seen.reserve(items_.size());
				for (const auto& existing : items_) seen.insert(existing.hash_hex);
				QVector<Ed2kSearchItem> fresh;
				for (const auto& item : items) {
					if (seen.contains(item.hash_hex)) continue;
					seen.insert(item.hash_hex);
					fresh.append(item);
				}
				if (fresh.isEmpty()) return;
				beginInsertRows({}, items_.size(), items_.size() + fresh.size() - 1);
				items_ += fresh;
				endInsertRows();
				Q_EMIT countChanged();
			}

			void Ed2kSearchResultModel::clear() {
				if (items_.isEmpty()) return;
				beginResetModel();
				items_.clear();
				endResetModel();
				Q_EMIT countChanged();
			}

		}  // namespace ed2k
	}  // namespace ui
}  // namespace gdl

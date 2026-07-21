#include "ed2k_shared_file_model.h"

#include <algorithm>

#include <QLocale>

#include "ed2k_link_builder.h"

namespace gdl {
	namespace ui {
		namespace ed2k {

			Ed2kSharedFileModel::Ed2kSharedFileModel(QObject* parent) : QAbstractListModel(parent) {}

			int Ed2kSharedFileModel::rowCount(const QModelIndex& parent) const {
				if (parent.isValid()) return 0;
				return items_.size();
			}

			QVariant Ed2kSharedFileModel::data(const QModelIndex& index, int role) const {
				if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) return {};
				const auto& item = items_.at(index.row());
				switch (role) {
					case kName: return item.name;
					case kPath: return item.path;
					case kSize: return item.size;
					case kSizeText: return QLocale().formattedDataSize(item.size);
					case kUploaded: return item.uploaded;
					case kUploadedText: return QLocale().formattedDataSize(item.uploaded);
					case kRequests: return item.requests;
					case kRawLink: return BuildRawLink(item.name, item.size, item.hash_hex);
					default: return {};
				}
			}

			QHash<int, QByteArray> Ed2kSharedFileModel::roleNames() const {
				return {
					{kName, "fileName"},     {kPath, "filePath"},           {kSize, "fileSize"},
					{kSizeText, "fileSizeText"}, {kUploaded, "uploaded"},   {kUploadedText, "uploadedText"},
					{kRequests, "requests"}, {kRawLink, "rawLink"},
				};
			}

			void Ed2kSharedFileModel::ResetItems(const QVector<Ed2kSharedItem>& items) {
				beginResetModel();
				items_ = items;
				std::stable_sort(items_.begin(), items_.end(),
								  [](const Ed2kSharedItem& a, const Ed2kSharedItem& b) { return a.requests > b.requests; });
				endResetModel();
				Q_EMIT countChanged();
			}

		}  // namespace ed2k
	}  // namespace ui
}  // namespace gdl

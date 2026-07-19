#pragma once

#include <QUrl>
#include <QString>
#include <optional>
#include <string>
#include <unordered_map>

// IsEd2kLink 已在 gdl::ui::browser 命名空间下声明(ed2k_link.h)，
// 与本文件同处一个命名空间，无需再做转发，直接引入声明即可在调用点使用。
#include "ed2k_link.h"

namespace gdl {
	namespace ui {
		namespace browser {

			inline std::optional<QString> NormalizeDownloadUrlForAria2(const QString& raw_url) {
				const QString trimmed = raw_url.trimmed();
				if (trimmed.isEmpty()) {
					return std::nullopt;
				}

				// magnet 链接没有 host 语义，保持原始格式交给 aria2。
				if (trimmed.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) {
					return trimmed;
				}

				const QUrl parsed = QUrl::fromUserInput(trimmed);
				const QString scheme = parsed.scheme().toLower();
				if (!parsed.isValid() || parsed.host().isEmpty() ||
					(scheme != QStringLiteral("http") && scheme != QStringLiteral("https") &&
					 scheme != QStringLiteral("ftp"))) {
					return std::nullopt;
				}

				return parsed.toString();
			}

			inline QString SuggestDownloadFileNameFromUrl(const QString& raw_url) {
				const QString trimmed = raw_url.trimmed();
				if (trimmed.isEmpty() || trimmed.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) {
					return {};
				}

				const QUrl parsed = QUrl::fromUserInput(trimmed);
				if (!parsed.isValid()) {
					return {};
				}

				return parsed.fileName(QUrl::FullyDecoded).trimmed();
			}

			inline void AddSuggestedOutOptionForUrl(
				std::unordered_multimap<std::string, std::string>& options, const QString& raw_url) {
				if (options.find("out") != options.end()) {
					return;
				}

				const QString file_name = SuggestDownloadFileNameFromUrl(raw_url);
				if (!file_name.isEmpty()) {
					options.emplace("out", file_name.toStdString());
				}
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl

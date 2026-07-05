#pragma once

#include <QUrl>
#include <QString>
#include <optional>

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

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl

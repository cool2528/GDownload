#include "ed2k_link.h"

#include <QRegularExpression>
#include <QStringList>
#include <QUrl>

namespace gdl {
	namespace ui {
		namespace browser {

			namespace {

				// md4 十六进制校验正则：严格 32 位十六进制字符。
				const QRegularExpression& Md4HexPattern() {
					static const QRegularExpression pattern(QStringLiteral("^[0-9A-Fa-f]{32}$"));
					return pattern;
				}

			}  // namespace

			Ed2kFileEntry ParseEd2kLink(const QString& link) {
				Ed2kFileEntry entry;

				const QString trimmed = link.trimmed();
				if (!trimmed.startsWith(QStringLiteral("ed2k://"), Qt::CaseInsensitive)) {
					return entry;
				}

				// ed2k://|file|name|size|md4|/ 按 '|' 切分共 6 段(首段为协议头，末段为空)。
				const QStringList tokens = trimmed.split(QLatin1Char('|'));
				if (tokens.size() < 5) {
					return entry;
				}

				if (tokens.at(1).compare(QStringLiteral("file"), Qt::CaseInsensitive) != 0) {
					return entry;
				}

				const QString decoded_name = QUrl::fromPercentEncoding(tokens.at(2).toUtf8());
				if (decoded_name.isEmpty()) {
					return entry;
				}

				bool size_ok = false;
				const std::int64_t size = tokens.at(3).toLongLong(&size_ok);
				if (!size_ok || size <= 0) {
					return entry;
				}

				const QString md4_candidate = tokens.at(4);
				if (!Md4HexPattern().match(md4_candidate).hasMatch()) {
					return entry;
				}

				entry.name = decoded_name;
				entry.size = size;
				entry.md4_hex = md4_candidate.toUpper();
				entry.valid = true;
				entry.raw = trimmed;
				return entry;
			}

			QVector<Ed2kFileEntry> ParseEd2kLinks(const QString& text) {
				QVector<Ed2kFileEntry> entries;

				// 兼容 \n 和 \r\n 换行，逐行解析并跳过空行/无效行。
				const QStringList lines = text.split(QRegularExpression(QStringLiteral("\r\n|\n")));
				for (const QString& line : lines) {
					const QString trimmed_line = line.trimmed();
					if (trimmed_line.isEmpty()) {
						continue;
					}

					const Ed2kFileEntry entry = ParseEd2kLink(trimmed_line);
					if (entry.valid) {
						entries.append(entry);
					}
				}

				return entries;
			}

			bool IsEd2kLink(const QString& url) {
				return url.trimmed().startsWith(QStringLiteral("ed2k://"), Qt::CaseInsensitive);
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl

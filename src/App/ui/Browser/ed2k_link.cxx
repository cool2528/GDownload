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
				// 链接来自网页/剪贴板等外部输入,百分号解码可能还原出路径分隔符或 ".."(如 %2F%2E%2E),
				// 文件名会被引擎拼进保存路径,必须拒绝任何可逃出目标目录的名字;
				// '|' 会破坏重建规范链接时的分段结构,一并拒绝
				if (decoded_name.contains(QLatin1Char('/')) || decoded_name.contains(QLatin1Char('\\')) ||
					decoded_name.contains(QLatin1Char('|')) || decoded_name == QStringLiteral("..") ||
					decoded_name == QStringLiteral(".")) {
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

			QString CanonicalizeEd2kLink(const QString& link) {
				// 复用 ParseEd2kLink 的解码 + 安全校验，再用解码后的文件名重建标准链接，
				// 交给引擎的链接文件名段即为可直接落盘的安全名字。
				const Ed2kFileEntry entry = ParseEd2kLink(link);
				if (!entry.valid) {
					return QString();
				}
				return QStringLiteral("ed2k://|file|%1|%2|%3|/")
					.arg(entry.name)
					.arg(entry.size)
					.arg(entry.md4_hex);
			}

			bool IsEd2kLink(const QString& url) {
				return url.trimmed().startsWith(QStringLiteral("ed2k://"), Qt::CaseInsensitive);
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl

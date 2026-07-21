#pragma once

#include <QString>
#include <QUrl>

namespace gdl {
	namespace ui {
		namespace ed2k {

			// 依据文件名/大小/十六进制哈希拼装标准 ed2k 文件链接，文件名做百分号编码。
			// 搜索结果模型与共享文件模型共用同一实现，避免拼装格式漂移。
			inline QString BuildRawLink(const QString& name, qint64 size, const QString& hash_hex) {
				const QString encoded = QString::fromUtf8(QUrl::toPercentEncoding(name));
				return QStringLiteral("ed2k://|file|%1|%2|%3|/").arg(encoded).arg(size).arg(hash_hex);
			}

		}  // namespace ed2k
	}  // namespace ui
}  // namespace gdl

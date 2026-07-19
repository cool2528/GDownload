#pragma once

#include <QString>
#include <QVector>

#include <cstdint>

namespace gdl {
	namespace ui {
		namespace browser {

			// 单条 ed2k file 链接解析结果。
			struct Ed2kFileEntry {
				QString name;			 // URL 解码后的文件名
				std::int64_t size = 0;	 // 文件大小(字节)
				QString md4_hex;		 // 32 位十六进制 MD4(大写)
				bool valid = false;	 // 是否为合法的 ed2k file 链接
				QString raw;			 // 原始链接(解析成功时回填)
			};

			// 解析单条 ed2k://|file|name|size|md4|/ 链接。
			// 非 file 类型或格式错误的链接返回 valid=false。
			Ed2kFileEntry ParseEd2kLink(const QString& link);

			// 从多行文本中解析出所有合法的 ed2k file 链接，忽略空行/无效行。
			QVector<Ed2kFileEntry> ParseEd2kLinks(const QString& text);

			// 判断字符串是否为 ed2k 链接(大小写不敏感，忽略首尾空白)。
			bool IsEd2kLink(const QString& url);

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl

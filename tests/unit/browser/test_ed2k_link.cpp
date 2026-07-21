#include <gtest/gtest.h>

#include "Browser/ed2k_link.h"

using gdl::ui::browser::CanonicalizeEd2kLink;
using gdl::ui::browser::Ed2kFileEntry;
using gdl::ui::browser::IsEd2kLink;
using gdl::ui::browser::ParseEd2kLink;
using gdl::ui::browser::ParseEd2kLinks;

TEST(Ed2kLinkTest, ParsesValidFileLink) {
	const Ed2kFileEntry entry =
		ParseEd2kLink(QStringLiteral("ed2k://|file|Ubuntu.iso|1500000000|A1B2C3D4E5F6A7B8C9D0E1F2A3B4C5D6|/"));

	ASSERT_TRUE(entry.valid);
	EXPECT_EQ(entry.name, QStringLiteral("Ubuntu.iso"));
	EXPECT_EQ(entry.size, 1500000000LL);
	EXPECT_EQ(entry.md4_hex, QStringLiteral("A1B2C3D4E5F6A7B8C9D0E1F2A3B4C5D6"));
}

TEST(Ed2kLinkTest, DecodesUrlEncodedFileName) {
	const Ed2kFileEntry entry =
		ParseEd2kLink(QStringLiteral("ed2k://|file|My%20File.mkv|100|00112233445566778899AABBCCDDEEFF|/"));

	ASSERT_TRUE(entry.valid);
	EXPECT_EQ(entry.name, QStringLiteral("My File.mkv"));
}

TEST(Ed2kLinkTest, RejectsNonFileLink) {
	const Ed2kFileEntry entry = ParseEd2kLink(QStringLiteral("ed2k://|server|1.2.3.4|4661|/"));

	EXPECT_FALSE(entry.valid);
}

TEST(Ed2kLinkTest, RejectsNonEd2kLink) {
	const Ed2kFileEntry entry = ParseEd2kLink(QStringLiteral("http://x/y"));

	EXPECT_FALSE(entry.valid);
}

TEST(Ed2kLinkTest, IsEd2kLinkTrimsAndIgnoresCase) {
	EXPECT_TRUE(IsEd2kLink(QStringLiteral("  ED2K://|file|a|1|x|/  ")));
	EXPECT_FALSE(IsEd2kLink(QStringLiteral("magnet:?xt=...")));
}

// 文件名会被拼进保存路径,任何解码后含路径分隔符或目录跳转的名字都必须拒绝
TEST(Ed2kLinkTest, RejectsPathTraversalFileName) {
	// 百分号编码还原出 "../x"
	EXPECT_FALSE(
		ParseEd2kLink(QStringLiteral("ed2k://|file|%2E%2E%2Fx|100|00112233445566778899AABBCCDDEEFF|/")).valid);
	// 反斜杠(Windows 分隔符)
	EXPECT_FALSE(
		ParseEd2kLink(QStringLiteral("ed2k://|file|a%5Cb.exe|100|00112233445566778899AABBCCDDEEFF|/")).valid);
	// 纯 ".."
	EXPECT_FALSE(
		ParseEd2kLink(QStringLiteral("ed2k://|file|..|100|00112233445566778899AABBCCDDEEFF|/")).valid);
	// 正常带点文件名不受影响
	EXPECT_TRUE(
		ParseEd2kLink(QStringLiteral("ed2k://|file|a.tar.gz|100|00112233445566778899AABBCCDDEEFF|/")).valid);
}

// CanonicalizeEd2kLink 把外部链接归一化为"文件名已解码 + 已安全校验"的标准链接,
// 供交给引擎前使用,防止 "My%20File.mkv" 之类的百分号编码名字直接落到磁盘上。
TEST(Ed2kLinkTest, CanonicalizeDecodesEncodedSpace) {
	const QString canonical =
		CanonicalizeEd2kLink(QStringLiteral("ed2k://|file|My%20File.mkv|100|00112233445566778899AABBCCDDEEFF|/"));

	// 规范化后链接中的文件名应为已解码的 "My File.mkv"(引擎会原样拼进保存路径)
	EXPECT_EQ(canonical,
			  QStringLiteral("ed2k://|file|My File.mkv|100|00112233445566778899AABBCCDDEEFF|/"));
}

TEST(Ed2kLinkTest, CanonicalizeDecodesCjkName) {
	// "%E6%98%A0%E7%94%BB.mkv" 解码为 CJK "映画.mkv"
	const QString canonical = CanonicalizeEd2kLink(QStringLiteral(
		"ed2k://|file|%E6%98%A0%E7%94%BB.mkv|100|00112233445566778899AABBCCDDEEFF|/"));

	EXPECT_EQ(canonical, QString::fromUtf8("ed2k://|file|\xE6\x98\xA0\xE7\x94\xBB.mkv|100|"
										   "00112233445566778899AABBCCDDEEFF|/"));
}

TEST(Ed2kLinkTest, CanonicalizeRejectsEncodedTraversal) {
	// 百分号编码的 "../"
	EXPECT_TRUE(
		CanonicalizeEd2kLink(QStringLiteral("ed2k://|file|%2e%2e%2fx|100|00112233445566778899AABBCCDDEEFF|/"))
			.isEmpty());
	// 编码反斜杠 "..\\"
	EXPECT_TRUE(
		CanonicalizeEd2kLink(QStringLiteral("ed2k://|file|..%5C|100|00112233445566778899AABBCCDDEEFF|/"))
			.isEmpty());
	// 编码正斜杠
	EXPECT_TRUE(
		CanonicalizeEd2kLink(QStringLiteral("ed2k://|file|a%2Fb|100|00112233445566778899AABBCCDDEEFF|/"))
			.isEmpty());
	// 非法链接
	EXPECT_TRUE(CanonicalizeEd2kLink(QStringLiteral("http://x/y")).isEmpty());
}

// 带 AICH 与源提示尾段的链接: 文件名解码消毒, 尾段原样保留
TEST(Ed2kLinkTest, CanonicalizePreservesTrailingSegments) {
	const QString in = QStringLiteral(
		"ed2k://|file|a%20b.mkv|123|00112233445566778899AABBCCDDEEFF|h=ABCDEFGH|/|sources,1.2.3.4:4662|/");
	const QString out = CanonicalizeEd2kLink(in);
	EXPECT_TRUE(out.contains(QStringLiteral("|a b.mkv|")) || out.contains(QStringLiteral("a b.mkv")));
	EXPECT_TRUE(out.contains(QStringLiteral("h=ABCDEFGH")));
	EXPECT_TRUE(out.contains(QStringLiteral("sources,1.2.3.4:4662")));
}

TEST(Ed2kLinkTest, ParsesMultipleLinesFromText) {
	const QVector<Ed2kFileEntry> entries = ParseEd2kLinks(QStringLiteral(
		"ed2k://|file|a|1|00112233445566778899AABBCCDDEEFF|/\n"
		"junk\n"
		"ed2k://|file|b|2|FFEEDDCCBBAA99887766554433221100|/"));

	ASSERT_EQ(entries.size(), 2);
	EXPECT_EQ(entries.at(0).name, QStringLiteral("a"));
	EXPECT_EQ(entries.at(1).name, QStringLiteral("b"));
}

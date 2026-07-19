#include <gtest/gtest.h>

#include "Browser/ed2k_link.h"

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

TEST(Ed2kLinkTest, ParsesMultipleLinesFromText) {
	const QVector<Ed2kFileEntry> entries = ParseEd2kLinks(QStringLiteral(
		"ed2k://|file|a|1|00112233445566778899AABBCCDDEEFF|/\n"
		"junk\n"
		"ed2k://|file|b|2|FFEEDDCCBBAA99887766554433221100|/"));

	ASSERT_EQ(entries.size(), 2);
	EXPECT_EQ(entries.at(0).name, QStringLiteral("a"));
	EXPECT_EQ(entries.at(1).name, QStringLiteral("b"));
}

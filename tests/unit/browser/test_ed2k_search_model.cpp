#include <gtest/gtest.h>

#include "Ed2k/ed2k_search_result_model.h"

using gdl::ui::ed2k::Ed2kSearchItem;
using gdl::ui::ed2k::Ed2kSearchResultModel;

// Reset 应按源数降序排列并暴露全部角色
TEST(Ed2kSearchResultModelTest, ResetSortsBySourcesDescending) {
	Ed2kSearchResultModel model;
	model.ResetItems({
		{QStringLiteral("low.bin"), 100, QStringLiteral("00112233445566778899AABBCCDDEEFF"), 3, 1},
		{QStringLiteral("high.bin"), 200, QStringLiteral("FFEEDDCCBBAA99887766554433221100"), 56, 41},
	});
	ASSERT_EQ(model.rowCount(), 2);
	const auto first = model.index(0, 0);
	EXPECT_EQ(model.data(first, Ed2kSearchResultModel::kName).toString(), QStringLiteral("high.bin"));
	EXPECT_EQ(model.data(first, Ed2kSearchResultModel::kSources).toUInt(), 56u);
	EXPECT_EQ(model.data(first, Ed2kSearchResultModel::kCompleteSources).toUInt(), 41u);
	EXPECT_EQ(model.data(first, Ed2kSearchResultModel::kRawLink).toString(),
			  QStringLiteral("ed2k://|file|high.bin|200|FFEEDDCCBBAA99887766554433221100|/"));
}

// Append 应按 hash 去重
TEST(Ed2kSearchResultModelTest, AppendDedupesByHash) {
	Ed2kSearchResultModel model;
	model.ResetItems({{QStringLiteral("a"), 1, QStringLiteral("00112233445566778899AABBCCDDEEFF"), 5, 0}});
	model.AppendItems({
		{QStringLiteral("a"), 1, QStringLiteral("00112233445566778899AABBCCDDEEFF"), 5, 0},   // 重复
		{QStringLiteral("b"), 2, QStringLiteral("FFEEDDCCBBAA99887766554433221100"), 9, 2},
	});
	EXPECT_EQ(model.rowCount(), 2);
}

// 文件名含需转义字符时 rawLink 做百分号编码
TEST(Ed2kSearchResultModelTest, RawLinkPercentEncodesName) {
	Ed2kSearchResultModel model;
	model.ResetItems({{QStringLiteral("a b|c.mkv"), 7, QStringLiteral("00112233445566778899AABBCCDDEEFF"), 1, 0}});
	const auto link = model.data(model.index(0, 0), Ed2kSearchResultModel::kRawLink).toString();
	EXPECT_FALSE(link.contains(QStringLiteral("a b")));
	EXPECT_TRUE(link.contains(QStringLiteral("a%20b%7Cc.mkv")));
}

#include <gtest/gtest.h>

#include "Ed2k/ed2k_shared_file_model.h"

using gdl::ui::ed2k::Ed2kSharedFileModel;
using gdl::ui::ed2k::Ed2kSharedItem;

// Reset 应按 requests 降序并暴露全部角色
TEST(Ed2kSharedFileModelTest, ResetSortsByRequestsDescending) {
	Ed2kSharedFileModel model;
	model.ResetItems({
		{QStringLiteral("cold.bin"), QStringLiteral("D:/share/cold.bin"), 100,
		 QStringLiteral("00112233445566778899AABBCCDDEEFF"), 10, 1},
		{QStringLiteral("hot.bin"), QStringLiteral("D:/share/hot.bin"), 200,
		 QStringLiteral("FFEEDDCCBBAA99887766554433221100"), 999, 57},
	});
	ASSERT_EQ(model.rowCount(), 2);
	const auto first = model.index(0, 0);
	EXPECT_EQ(model.data(first, Ed2kSharedFileModel::kName).toString(), QStringLiteral("hot.bin"));
	EXPECT_EQ(model.data(first, Ed2kSharedFileModel::kRequests).toUInt(), 57u);
	EXPECT_EQ(model.data(first, Ed2kSharedFileModel::kRawLink).toString(),
			  QStringLiteral("ed2k://|file|hot.bin|200|FFEEDDCCBBAA99887766554433221100|/"));
}

// 文件名需转义字符时 rawLink 百分号编码(与搜索模型同一实现)
TEST(Ed2kSharedFileModelTest, RawLinkPercentEncodesName) {
	Ed2kSharedFileModel model;
	model.ResetItems({{QStringLiteral("a b.mkv"), QStringLiteral("D:/s/a b.mkv"), 7,
					   QStringLiteral("00112233445566778899AABBCCDDEEFF"), 0, 0}});
	EXPECT_TRUE(model.data(model.index(0, 0), Ed2kSharedFileModel::kRawLink)
					.toString().contains(QStringLiteral("a%20b.mkv")));
}

#include <gtest/gtest.h>

#include "Ed2k/ed2k_server_list_model.h"

using gdl::ui::ed2k::Ed2kServerItem;
using gdl::ui::ed2k::Ed2kServerListModel;

// 已连接行置顶，其余按用户数降序
TEST(Ed2kServerListModelTest, ConnectedFirstThenUsersDescending) {
	Ed2kServerListModel model;
	model.ResetItems({
		{QStringLiteral("big"), QStringLiteral("1.1.1.1"), 4661, 900000, 100, 0, false},
		{QStringLiteral("mine"), QStringLiteral("2.2.2.2"), 4661, 5, 1, 0, true},
		{QStringLiteral("small"), QStringLiteral("3.3.3.3"), 4661, 10, 2, 0, false},
	});
	ASSERT_EQ(model.rowCount(), 3);
	EXPECT_EQ(model.data(model.index(0, 0), Ed2kServerListModel::kName).toString(), QStringLiteral("mine"));
	EXPECT_EQ(model.data(model.index(1, 0), Ed2kServerListModel::kName).toString(), QStringLiteral("big"));
	EXPECT_EQ(model.data(model.index(0, 0), Ed2kServerListModel::kAddress).toString(),
			  QStringLiteral("2.2.2.2:4661"));
}

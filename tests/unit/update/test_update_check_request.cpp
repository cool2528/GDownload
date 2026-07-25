#include <gtest/gtest.h>
#include "App/ui/update/update_check_request.h"

namespace gdl::update {
namespace {

TEST(UpdateCheckRequestTest, StartsSilentCheckWhenIdle) {
	const auto decision = CoalesceUpdateCheckRequest(false, true, true);

	EXPECT_TRUE(decision.start_new_check);
	EXPECT_TRUE(decision.silent);
}

TEST(UpdateCheckRequestTest, StartsManualCheckWhenIdle) {
	const auto decision = CoalesceUpdateCheckRequest(false, true, false);

	EXPECT_TRUE(decision.start_new_check);
	EXPECT_FALSE(decision.silent);
}

TEST(UpdateCheckRequestTest, ManualRequestUpgradesInFlightSilentCheck) {
	// 手动点击时已有静默检查在途:不发起第二次底层检查,但升级在途检查为
	// 非静默,确保其完成回执发射给设置页(否则按钮永久停留在忙碌态)
	const auto decision = CoalesceUpdateCheckRequest(true, true, false);

	EXPECT_FALSE(decision.start_new_check);
	EXPECT_FALSE(decision.silent);
}

TEST(UpdateCheckRequestTest, SilentRequestKeepsInFlightManualCheckNonSilent) {
	// 在途手动检查不被后续静默请求降级,完成回执仍须发射
	const auto decision = CoalesceUpdateCheckRequest(true, false, true);

	EXPECT_FALSE(decision.start_new_check);
	EXPECT_FALSE(decision.silent);
}

}  // namespace
}  // namespace gdl::update

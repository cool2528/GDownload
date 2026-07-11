#include <gtest/gtest.h>
#include <future>
#include <stdexcept>
#include "Aria2CManager/tracker_sync_gate.h"

using gdl::engine::TrackerSyncGate;

TEST(TrackerSyncGateTest, StoppingPreventsDispatch) { TrackerSyncGate gate; gate.BeginStopping(); EXPECT_FALSE(gate.Dispatch([]{})); }
TEST(TrackerSyncGateTest, TakingFuturePreventsReplacement) { TrackerSyncGate gate; std::promise<void> release; auto ready=release.get_future().share(); ASSERT_TRUE(gate.Dispatch([ready]{ready.wait();})); auto future=gate.BeginStoppingAndTakeFuture(); EXPECT_TRUE(future.valid()); EXPECT_FALSE(gate.Dispatch([]{})); release.set_value(); future.get(); }
TEST(TrackerSyncGateTest, ExceptionResetsRunningState) { TrackerSyncGate gate; ASSERT_TRUE(gate.Dispatch([]{throw std::runtime_error("boom");})); auto future=gate.TakeFuture(); ASSERT_TRUE(future.valid()); EXPECT_THROW(future.get(),std::runtime_error); EXPECT_FALSE(gate.IsRunning()); EXPECT_TRUE(gate.Dispatch([]{})); gate.BeginStoppingAndTakeFuture().get(); }

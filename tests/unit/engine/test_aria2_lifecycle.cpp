#include <gtest/gtest.h>

#include <chrono>
#include <type_traits>

#include "Aria2CManager/aria2c_lifecycle.h"

namespace {
using namespace gdl::engine;

static_assert(std::is_abstract_v<IAria2ProcessLifecycle>);
static_assert(std::is_abstract_v<IAria2RpcLifecycle>);
static_assert(std::is_same_v<decltype(Aria2StartupResult::pid), int64_t>);
static_assert(std::is_same_v<decltype(&IAria2ProcessLifecycle::Execute),
	int64_t (IAria2ProcessLifecycle::*)(const gdl::String_View&, const std::vector<gdl::String>&)>);
static_assert(std::is_same_v<decltype(&IAria2ProcessLifecycle::IsAlive),
	bool (IAria2ProcessLifecycle::*)(int64_t)>);
static_assert(std::is_same_v<decltype(&IAria2ProcessLifecycle::Shutdown),
	void (IAria2ProcessLifecycle::*)(int64_t, int)>);
static_assert(std::is_same_v<decltype(&IAria2RpcLifecycle::SetStateCallback),
	void (IAria2RpcLifecycle::*)(IAria2RpcLifecycle::StateCallback)>);
static_assert(std::is_same_v<decltype(&IAria2RpcLifecycle::ClearStateCallback),
	void (IAria2RpcLifecycle::*)()>);
static_assert(std::is_same_v<decltype(&IAria2RpcLifecycle::Open), void (IAria2RpcLifecycle::*)()>);
static_assert(std::is_same_v<decltype(&IAria2RpcLifecycle::IsReady),
	bool (IAria2RpcLifecycle::*)() const>);
static_assert(std::is_same_v<decltype(&IAria2RpcLifecycle::RequestShutdown),
	bool (IAria2RpcLifecycle::*)()>);
static_assert(std::is_same_v<decltype(&IAria2RpcLifecycle::DisableReconnect),
	void (IAria2RpcLifecycle::*)()>);
static_assert(std::is_same_v<decltype(&IAria2RpcLifecycle::Disconnect), void (IAria2RpcLifecycle::*)()>);

class FakeRpcLifecycle final : public IAria2RpcLifecycle {
   public:
	void SetStateCallback(StateCallback callback) override { callback_ = std::move(callback); }
	void ClearStateCallback() override { callback_ = {}; }
	void Open() override {}
	bool IsReady() const override { return false; }
	bool RequestShutdown() override { return true; }
	void DisableReconnect() override {}
	void Disconnect() override {}
	void Notify(bool ready) { if (callback_) callback_(ready); }

   private:
	StateCallback callback_;
};

TEST(Aria2LifecycleContractTest, DefaultTimingIsBounded) {
	constexpr Aria2LifecycleTiming timing;
	EXPECT_EQ(timing.readiness_timeout, std::chrono::milliseconds(5000));
	EXPECT_EQ(timing.readiness_poll_interval, std::chrono::milliseconds(50));
	EXPECT_EQ(timing.shutdown_grace_period, std::chrono::milliseconds(2000));
	EXPECT_EQ(timing.liveness_check_interval, std::chrono::milliseconds(1000));
	EXPECT_GT(timing.readiness_timeout, timing.readiness_poll_interval);
}

TEST(Aria2LifecycleContractTest, StartupResultDefaultsToNoErrorAndNoProcess) {
	constexpr Aria2StartupResult result;
	EXPECT_EQ(result.error, Aria2StartupError::kNone);
	EXPECT_EQ(result.pid, 0);
}

TEST(Aria2LifecycleContractTest, ClearingStateCallbackPreventsFutureNotifications) {
	FakeRpcLifecycle rpc;
	int notifications = 0;
	rpc.SetStateCallback([&](bool) { ++notifications; });
	rpc.Notify(true);
	EXPECT_EQ(notifications, 1);
	rpc.ClearStateCallback();
	rpc.Notify(false);
	EXPECT_EQ(notifications, 1);
}

}  // namespace

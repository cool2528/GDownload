#include <gtest/gtest.h>

#include <chrono>
#include <atomic>
#include <future>
#include <thread>
#include <type_traits>

#include "Aria2CManager/aria2c_lifecycle.h"
#include "Aria2CManager/aria2c_lifecycle_adapters.h"
#include "Aria2CManager/aria2c_lifecycle_controller.h"
#include "Aria2CManager/websocket_client.h"
#include "view/engine_startup_policy.h"
#include "Aria2CManager/poll_drain_gate.h"

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
static_assert(std::is_same_v<decltype(&Aria2cWebSocketClient::IsConnected),
	bool (Aria2cWebSocketClient::*)() const>);
static_assert(std::is_same_v<decltype(&Aria2cWebSocketClient::DisableAutoReconnect),
	void (Aria2cWebSocketClient::*)()>);
static_assert(std::is_constructible_v<Aria2RpcLifecycleAdapter, Aria2cWebSocketClient&>);

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

class FakeProcessLifecycle final : public IAria2ProcessLifecycle {
   public:
	int64_t Execute(const gdl::String_View&, const std::vector<gdl::String>&) override {
		events.push_back("execute");
		return pid;
	}
	bool IsAlive(int64_t) override {
		events.push_back("alive");
		return alive_checks++ < alive_for_checks;
	}
	void Shutdown(int64_t, int grace_ms) override {
		events.push_back("process-shutdown");
		shutdown_grace_ms = grace_ms;
		++shutdown_calls;
	}

	int64_t pid{42};
	int alive_for_checks{100};
	int alive_checks{0};
	int shutdown_calls{0};
	int shutdown_grace_ms{0};
	std::vector<std::string> events;
};

class FakeControllerRpc final : public IAria2RpcLifecycle {
   public:
	void SetStateCallback(StateCallback callback) override { callback_ = std::move(callback); }
	void ClearStateCallback() override { events.push_back("clear-callback"); callback_ = {}; }
	void Open() override { events.push_back("open"); ++open_calls; }
	bool IsReady() const override { return ready_checks++ >= ready_after_checks; }
	bool RequestShutdown() override { events.push_back("rpc-shutdown"); ++shutdown_calls; return shutdown_result; }
	void DisableReconnect() override { events.push_back("disable-reconnect"); ++disable_calls; }
	void Disconnect() override { events.push_back("disconnect"); ++disconnect_calls; }

	mutable int ready_checks{0};
	int ready_after_checks{0};
	int open_calls{0};
	int shutdown_calls{0};
	int disable_calls{0};
	int disconnect_calls{0};
	bool shutdown_result{true};
	std::vector<std::string> events;
	StateCallback callback_;
};

Aria2LifecycleTiming FastTiming() {
	return {.readiness_timeout = std::chrono::milliseconds(50),
		.readiness_poll_interval = std::chrono::milliseconds(1),
		.shutdown_grace_period = std::chrono::milliseconds(7),
		.liveness_check_interval = std::chrono::milliseconds(1)};
}

TEST(Aria2LifecycleControllerTest, LaunchFailureDoesNotOpenRpc) {
	FakeProcessLifecycle process;
	FakeControllerRpc rpc;
	process.pid = 0;
	Aria2LifecycleController controller(process, rpc, FastTiming());

	const auto result = controller.Start("aria2c", {});

	EXPECT_EQ(result.error, Aria2StartupError::kLaunch);
	EXPECT_EQ(result.state, Aria2LifecycleState::kFailed);
	EXPECT_EQ(rpc.open_calls, 0);
}

TEST(Aria2LifecycleControllerTest, BecomesReadyOnlyAfterRpcIsReady) {
	FakeProcessLifecycle process;
	FakeControllerRpc rpc;
	rpc.ready_after_checks = 2;
	Aria2LifecycleController controller(process, rpc, FastTiming());

	const auto result = controller.Start("aria2c", {});

	EXPECT_EQ(result.error, Aria2StartupError::kNone);
	EXPECT_EQ(controller.State(), Aria2LifecycleState::kReady);
	EXPECT_EQ(controller.Pid(), 42);
	EXPECT_EQ(rpc.open_calls, 1);
}

TEST(Aria2LifecycleControllerTest, TimeoutCleansUpRpcAndProcess) {
	FakeProcessLifecycle process;
	FakeControllerRpc rpc;
	rpc.ready_after_checks = 1000;
	Aria2LifecycleController controller(process, rpc, FastTiming());

	const auto result = controller.Start("aria2c", {});

	EXPECT_EQ(result.error, Aria2StartupError::kTimeout);
	EXPECT_EQ(process.shutdown_calls, 1);
	EXPECT_EQ(rpc.disable_calls, 1);
	EXPECT_EQ(rpc.disconnect_calls, 1);
}

TEST(Aria2LifecycleControllerTest, ProcessExitBeforeReadyIsReported) {
	FakeProcessLifecycle process;
	FakeControllerRpc rpc;
	process.alive_for_checks = 0;
	rpc.ready_after_checks = 1000;
	Aria2LifecycleController controller(process, rpc, FastTiming());

	const auto result = controller.Start("aria2c", {});

	EXPECT_EQ(result.error, Aria2StartupError::kExitedBeforeReady);
	EXPECT_EQ(controller.State(), Aria2LifecycleState::kFailed);
}

TEST(Aria2LifecycleControllerTest, RuntimeExitMarksEngineFailedWithoutRestart) {
	FakeProcessLifecycle process;
	FakeControllerRpc rpc;
	Aria2LifecycleController controller(process, rpc, FastTiming());
	ASSERT_EQ(controller.Start("aria2c", {}).error, Aria2StartupError::kNone);
	process.alive_for_checks = process.alive_checks;

	EXPECT_FALSE(controller.CheckLiveness());
	EXPECT_EQ(controller.State(), Aria2LifecycleState::kFailed);
	EXPECT_EQ(rpc.disable_calls, 1);
	EXPECT_EQ(rpc.disconnect_calls, 1);
	EXPECT_EQ(rpc.open_calls, 1);
}

TEST(Aria2LifecycleControllerTest, StopIsIdempotentAndAlwaysStopsProcessAfterRpc) {
	FakeProcessLifecycle process;
	FakeControllerRpc rpc;
	rpc.shutdown_result = false;
	Aria2LifecycleController controller(process, rpc, FastTiming());
	ASSERT_EQ(controller.Start("aria2c", {}).error, Aria2StartupError::kNone);

	controller.Stop();
	controller.Stop();

	EXPECT_EQ(rpc.shutdown_calls, 1);
	EXPECT_EQ(process.shutdown_calls, 1);
	EXPECT_EQ(process.shutdown_grace_ms, 7);
	EXPECT_EQ(rpc.disconnect_calls, 1);
	EXPECT_EQ(controller.State(), Aria2LifecycleState::kStopped);
}

TEST(Aria2LifecycleControllerTest, LivenessCheckAfterStopCannotOverwriteStoppedState) {
	FakeProcessLifecycle process;
	FakeControllerRpc rpc;
	Aria2LifecycleController controller(process, rpc, FastTiming());
	ASSERT_EQ(controller.Start("aria2c", {}).error, Aria2StartupError::kNone);
	controller.Stop();
	EXPECT_FALSE(controller.CheckLiveness());
	EXPECT_EQ(controller.State(), Aria2LifecycleState::kStopped);
}

TEST(EngineStartupPolicyTest, FailureSkipsBrowserInitializationAndMarksUnavailable) {
	int browser_init_calls = 0;
	int unavailable_calls = 0;
	const bool started = gdl::ui::RunEngineStartupPolicy([] { return false; },
		[&] { ++browser_init_calls; return true; }, [] {}, [&] { ++unavailable_calls; });
	EXPECT_FALSE(started);
	EXPECT_EQ(browser_init_calls, 0);
	EXPECT_EQ(unavailable_calls, 1);
}

TEST(EngineStartupPolicyTest, SuccessInitializesBrowser) {
	int browser_init_calls = 0;
	int unavailable_calls = 0;
	const bool started = gdl::ui::RunEngineStartupPolicy([] { return true; },
		[&] { ++browser_init_calls; return true; }, [] {}, [&] { ++unavailable_calls; });
	EXPECT_TRUE(started);
	EXPECT_EQ(browser_init_calls, 1);
	EXPECT_EQ(unavailable_calls, 0);
}

TEST(EngineStartupPolicyTest, BrowserInitFailureStopsEngineAndMarksUnavailable) {
	int stop_calls = 0;
	int unavailable_calls = 0;
	EXPECT_FALSE(gdl::ui::RunEngineStartupPolicy([] { return true; }, [] { return false; },
		[&] { ++stop_calls; }, [&] { ++unavailable_calls; }));
	EXPECT_EQ(stop_calls, 1);
	EXPECT_EQ(unavailable_calls, 1);
}

TEST(PollDrainGateTest, StoppingRejectsQueuedPollsAndWaitsForActivePoll) {
	PollDrainGate gate;
	auto active = gate.TryEnter();
	ASSERT_TRUE(active.has_value());
	auto stopping = std::async(std::launch::async, [&] { gate.StopAndDrain(); });
	EXPECT_EQ(stopping.wait_for(std::chrono::milliseconds(20)), std::future_status::timeout);
	EXPECT_FALSE(gate.TryEnter().has_value());
	active.reset();
	EXPECT_EQ(stopping.wait_for(std::chrono::seconds(1)), std::future_status::ready);
}

class FakeRpcClientBackend final : public detail::IAria2RpcClientBackend {
   public:
	void SetStateCallback(StateCallback callback) override { callback_ = std::move(callback); }
	void ClearStateCallback() override { callback_ = {}; }
	void Open() override {}
	bool IsConnected() const override { return connected_; }
	gdl::Result<bool> Shutdown() override { return shutdown_result_; }
	void DisableAutoReconnect() override {}
	void Disconnect() override {}
	void Notify(State state) { if (callback_) callback_(state, {}); }

	bool connected_{false};
	gdl::Result<bool> shutdown_result_{true};
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

TEST(Aria2LifecycleAdapterTest, MapsTransportStatesToReadiness) {
	EXPECT_TRUE(detail::IsReadyState(State::kConnected));
	EXPECT_FALSE(detail::IsReadyState(State::kClosed));
	EXPECT_FALSE(detail::IsReadyState(State::kError));
}

TEST(Aria2LifecycleAdapterTest, ConvertsShutdownResultToBoolean) {
	EXPECT_TRUE(detail::ShutdownSucceeded(gdl::Result<bool>(true)));
	EXPECT_FALSE(detail::ShutdownSucceeded(gdl::Result<bool>(false)));
	EXPECT_FALSE(detail::ShutdownSucceeded(
		gdl::Result<bool>(gdl::MakeFail(static_cast<std::int64_t>(gdl::ErrorType::kUnknownError)))));
}

TEST(Aria2LifecycleAdapterTest, ClearWaitsForRunningCallbackAndPreventsNewInvocation) {
	detail::SynchronizedStateCallback callback;
	std::promise<void> entered;
	std::promise<void> release;
	auto release_future = release.get_future().share();
	std::atomic<int> calls{0};
	callback.Set([&](State, std::string) {
		++calls;
		entered.set_value();
		release_future.wait();
	});

	std::thread notifier([&] { callback.Invoke(State::kConnected, {}); });
	entered.get_future().wait();
	auto clearing = std::async(std::launch::async, [&] { callback.Clear(); });
	EXPECT_EQ(clearing.wait_for(std::chrono::milliseconds(20)), std::future_status::timeout);
	release.set_value();
	EXPECT_EQ(clearing.wait_for(std::chrono::seconds(1)), std::future_status::ready);
	notifier.join();
	callback.Invoke(State::kClosed, {});
	EXPECT_EQ(calls.load(), 1);
}

TEST(Aria2LifecycleAdapterTest, CallbackCanClearItselfWithoutDeadlock) {
	detail::SynchronizedStateCallback callback;
	std::atomic<int> calls{0};
	callback.Set([&](State, std::string) {
		++calls;
		callback.Clear();
	});
	callback.Invoke(State::kConnected, {});
	callback.Invoke(State::kClosed, {});
	EXPECT_EQ(calls.load(), 1);
}

TEST(Aria2LifecycleAdapterTest, DisablingReconnectUpdatesStateAndCancelsPendingWork) {
	boost::asio::io_context ioc;
	auto websocket = std::make_shared<WebSocketClient>(ioc);
	websocket->setAutoReconnect(true, -1, 2);
	ASSERT_TRUE(websocket->isAutoReconnectEnabled());
	websocket->disableAutoReconnect();
	ioc.run();
	EXPECT_FALSE(websocket->isAutoReconnectEnabled());
}

TEST(Aria2LifecycleAdapterTest, RejectsNullClient) {
	EXPECT_THROW(Aria2RpcLifecycleAdapter(std::shared_ptr<Aria2cWebSocketClient>{}), std::invalid_argument);
}

TEST(Aria2LifecycleAdapterTest, DestructionClearsStateCallback) {
	auto backend = std::make_shared<FakeRpcClientBackend>();
	int notifications = 0;
	{
		Aria2RpcLifecycleAdapter adapter(backend);
		adapter.SetStateCallback([&](bool) { ++notifications; });
		backend->Notify(State::kConnected);
		EXPECT_EQ(notifications, 1);
	}
	backend->Notify(State::kClosed);
	EXPECT_EQ(notifications, 1);
}

}  // namespace

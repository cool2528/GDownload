#include <gtest/gtest.h>

#include <chrono>
#include <atomic>
#include <future>
#include <thread>
#include <type_traits>

#include "Aria2CManager/aria2c_lifecycle.h"
#include "Aria2CManager/aria2c_lifecycle_adapters.h"
#include "Aria2CManager/websocket_client.h"

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

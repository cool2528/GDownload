#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

#include "Engine_export.h"
#include "globalTypes.h"

namespace gdl::engine {

	enum class Aria2LifecycleState {
		kStopped,
		kStarting,
		kReady,
		kFailed,
		kStopping,
	};

	enum class Aria2StartupError {
		kNone,
		kLaunch,
		kExitedBeforeReady,
		kTimeout,
	};

	struct Aria2StartupResult {
		Aria2LifecycleState state{Aria2LifecycleState::kStopped};
		Aria2StartupError error{Aria2StartupError::kNone};
		int64_t pid{0};
	};

	struct Aria2LifecycleTiming {
		std::chrono::milliseconds readiness_timeout{5000};
		std::chrono::milliseconds readiness_poll_interval{50};
		std::chrono::milliseconds shutdown_grace_period{2000};
		std::chrono::milliseconds liveness_check_interval{1000};
	};

	class IAria2ProcessLifecycle {
	   public:
		virtual ~IAria2ProcessLifecycle() = default;
		virtual int64_t Execute(const String_View& command, const std::vector<String>& arguments) = 0;
		virtual bool IsAlive(int64_t pid) = 0;
		virtual void Shutdown(int64_t pid, int grace_ms) = 0;
	};

	class IAria2RpcLifecycle {
	   public:
		using StateCallback = std::function<void(bool)>;

		virtual ~IAria2RpcLifecycle() = default;
		virtual void SetStateCallback(StateCallback callback) = 0;
		// 返回前必须与正在执行的状态回调完成序列化；返回后不得再启动先前注册的回调，
		// 以保证回调捕获对象可以安全销毁。
		virtual void ClearStateCallback() = 0;
		virtual void Open() = 0;
		virtual bool IsReady() const = 0;
		virtual bool RequestShutdown() = 0;
		virtual void DisableReconnect() = 0;
		virtual void Disconnect() = 0;
	};

}  // namespace gdl::engine

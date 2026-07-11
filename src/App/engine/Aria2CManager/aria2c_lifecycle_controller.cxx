#include "aria2c_lifecycle_controller.h"

#include <thread>

namespace gdl::engine {

	Aria2LifecycleController::Aria2LifecycleController(IAria2ProcessLifecycle& process,
		IAria2RpcLifecycle& rpc, Aria2LifecycleTiming timing)
		: process_(process), rpc_(rpc), timing_(timing) {}

	Aria2StartupResult Aria2LifecycleController::Start(const String_View& command,
		const std::vector<String>& arguments) {
		{
			std::lock_guard lock(mutex_);
			state_ = Aria2LifecycleState::kStarting;
			stopped_ = false;
			pid_ = process_.Execute(command, arguments);
			if (pid_ <= 0) {
				pid_ = 0;
				state_ = Aria2LifecycleState::kFailed;
				return {state_, Aria2StartupError::kLaunch, pid_};
			}
		}

		rpc_.Open();
		const auto deadline = std::chrono::steady_clock::now() + timing_.readiness_timeout;
		while (std::chrono::steady_clock::now() < deadline) {
			if (rpc_.IsReady()) {
				std::lock_guard lock(mutex_);
				state_ = Aria2LifecycleState::kReady;
				return {state_, Aria2StartupError::kNone, pid_};
			}
			if (!process_.IsAlive(Pid())) {
				CleanupFailedStart();
				return {Aria2LifecycleState::kFailed, Aria2StartupError::kExitedBeforeReady, Pid()};
			}
			std::this_thread::sleep_for(timing_.readiness_poll_interval);
		}

		CleanupFailedStart();
		return {Aria2LifecycleState::kFailed, Aria2StartupError::kTimeout, Pid()};
	}

	void Aria2LifecycleController::CleanupFailedStart() {
		rpc_.ClearStateCallback();
		rpc_.DisableReconnect();
		const auto pid = Pid();
		if (pid > 0) process_.Shutdown(pid, static_cast<int>(timing_.shutdown_grace_period.count()));
		rpc_.Disconnect();
		std::lock_guard lock(mutex_);
		state_ = Aria2LifecycleState::kFailed;
		stopped_ = true;
	}

	bool Aria2LifecycleController::CheckLiveness() {
		std::lock_guard operation_lock(operation_mutex_);
		const auto pid = Pid();
		if (State() != Aria2LifecycleState::kReady || process_.IsAlive(pid)) return State() == Aria2LifecycleState::kReady;
		rpc_.ClearStateCallback();
		rpc_.DisableReconnect();
		rpc_.Disconnect();
		std::lock_guard lock(mutex_);
		state_ = Aria2LifecycleState::kFailed;
		return false;
	}

	void Aria2LifecycleController::Stop() {
		std::lock_guard operation_lock(operation_mutex_);
		int64_t pid = 0;
		{
			std::lock_guard lock(mutex_);
			if (stopped_) return;
			stopped_ = true;
			state_ = Aria2LifecycleState::kStopping;
			pid = pid_;
		}
		rpc_.ClearStateCallback();
		rpc_.DisableReconnect();
		rpc_.RequestShutdown();
		if (pid > 0) process_.Shutdown(pid, static_cast<int>(timing_.shutdown_grace_period.count()));
		rpc_.Disconnect();
		std::lock_guard lock(mutex_);
		state_ = Aria2LifecycleState::kStopped;
		pid_ = 0;
	}

	Aria2LifecycleState Aria2LifecycleController::State() const {
		std::lock_guard lock(mutex_);
		return state_;
	}

	int64_t Aria2LifecycleController::Pid() const {
		std::lock_guard lock(mutex_);
		return pid_;
	}

}  // namespace gdl::engine

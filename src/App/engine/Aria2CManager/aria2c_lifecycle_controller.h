#pragma once

#include <mutex>

#include "aria2c_lifecycle.h"

namespace gdl::engine {

	class Engine_API Aria2LifecycleController final {
	   public:
		Aria2LifecycleController(IAria2ProcessLifecycle& process, IAria2RpcLifecycle& rpc,
			Aria2LifecycleTiming timing = {});

		Aria2StartupResult Start(const String_View& command, const std::vector<String>& arguments);
		bool CheckLiveness();
		void Stop();
		Aria2LifecycleState State() const;
		int64_t Pid() const;
		const Aria2LifecycleTiming& Timing() const { return timing_; }

	   private:
		void CleanupFailedStart();

		IAria2ProcessLifecycle& process_;
		IAria2RpcLifecycle& rpc_;
		Aria2LifecycleTiming timing_;
		mutable std::mutex mutex_;
		std::mutex operation_mutex_;
		Aria2LifecycleState state_{Aria2LifecycleState::kStopped};
		int64_t pid_{0};
		bool stopped_{false};
	};

}  // namespace gdl::engine

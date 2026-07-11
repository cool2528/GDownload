#pragma once
#include <atomic>
#include <future>
#include <mutex>
#include <utility>

namespace gdl::engine {
class TrackerSyncGate {
 public:
  template <typename Work>
  bool Dispatch(Work&& work) {
    if (stopping_.load()) return false;
    std::lock_guard lock(mutex_);
    if (stopping_.load() || running_.load()) return false;
    running_.store(true);
    future_ = std::async(std::launch::async, [this, task = std::forward<Work>(work)]() mutable {
      struct Reset { std::atomic_bool& value; ~Reset(){ value.store(false); } } reset{running_};
      task();
    });
    return true;
  }
  void BeginStopping() { stopping_.store(true); }
  std::future<void> BeginStoppingAndTakeFuture() { stopping_.store(true); return TakeFuture(); }
  std::future<void> TakeFuture() { std::lock_guard lock(mutex_); return std::move(future_); }
  bool IsRunning() const { return running_.load(); }
 private:
  std::atomic_bool stopping_{false};
  std::atomic_bool running_{false};
  std::mutex mutex_;
  std::future<void> future_;
};
}

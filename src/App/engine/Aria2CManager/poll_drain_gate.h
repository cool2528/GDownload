#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>

namespace gdl::engine {
	class PollDrainGate final {
	   public:
		class Entry final {
		   public:
			explicit Entry(PollDrainGate& gate) : gate_(&gate) {}
			Entry(Entry&& other) noexcept : gate_(other.gate_) { other.gate_ = nullptr; }
			Entry& operator=(Entry&&) = delete;
			Entry(const Entry&) = delete;
			Entry& operator=(const Entry&) = delete;
			~Entry() { if (gate_) gate_->Leave(); }
		   private:
			PollDrainGate* gate_;
		};

		std::optional<Entry> TryEnter() {
			std::lock_guard lock(mutex_);
			if (stopping_) return std::nullopt;
			++active_;
			return std::optional<Entry>(std::in_place, *this);
		}
		void StopAndDrain() {
			std::unique_lock lock(mutex_);
			stopping_ = true;
			idle_.wait(lock, [this] { return active_ == 0; });
		}
	   private:
		void Leave() {
			std::lock_guard lock(mutex_);
			if (--active_ == 0) idle_.notify_all();
		}
		std::mutex mutex_;
		std::condition_variable idle_;
		std::size_t active_{0};
		bool stopping_{false};
	};
}

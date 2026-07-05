/**
 * @brief asio 异步定时器
 */

#pragma once
#include <boost/asio.hpp>
#include <chrono>
#include <functional>
namespace gdl {
	namespace engine {

		using TimeOutCallback = std::function<void()>;
		class AsyncTimer {
		   public:
			explicit AsyncTimer(boost::asio::io_context& io)
				: strand_(boost::asio::make_strand(io)), timer_(strand_) {}
			void Start(const TimeOutCallback& cb, std::chrono::steady_clock::duration interval, bool repeat = false) {
				// 整体投递到 strand，避免与 io 线程上的 async_wait 回调竞态（E2）
				boost::asio::post(strand_, [this, cb, interval, repeat] {
					interval_		  = interval;
					timeout_callback_ = cb;
					repeat_			  = repeat;
					Next();
				});
			}
			void Stop() {
				// 与 Start/Next 同在 strand 上串行，杜绝 cancel 与 rearm 并发（E2）
				boost::asio::post(strand_, [this] {
					repeat_ = false;
					timer_.cancel();
				});
			}

		   private:
			void Next() {
				timer_.expires_after(interval_);
				timer_.async_wait([this](const boost::system::error_code& error) {
					if (!error) {
						if (timeout_callback_) {
							timeout_callback_();
						}
						if (repeat_) {
							Next();
						}
					}
				});
			}

		   private:
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			TimeOutCallback timeout_callback_{nullptr};
			std::chrono::steady_clock::duration interval_;
			boost::asio::steady_timer timer_;
			bool repeat_{false};
		};
	}  // namespace engine
}  // namespace gdl

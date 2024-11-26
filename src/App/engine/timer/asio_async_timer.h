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
			explicit AsyncTimer(boost::asio::io_context& io) : timer_(io) {}
			void Start(const TimeOutCallback& cb, std::chrono::steady_clock::duration interval, bool repeat = false) {
				interval_		  = interval;
				timeout_callback_ = cb;
				repeat_			  = repeat;
				Next();
			}
			void Stop() { timer_.cancel(); }

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
			TimeOutCallback timeout_callback_{nullptr};
			std::chrono::steady_clock::duration interval_;
			boost::asio::steady_timer timer_;
			bool repeat_{false};
		};
	}  // namespace engine
}  // namespace gdl

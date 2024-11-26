/**
 * @brief 每日任务定时器
 */

#pragma once
#include <boost/asio.hpp>
#include <chrono>
#include <functional>
namespace gdl {
	namespace engine {

		using DailyTaskTimeOutCallback = std::function<void()>;
		class DailyTaskTimer {
		   public:
			explicit DailyTaskTimer(boost::asio::io_context& io) : timer_(io) {}
			void Start(const DailyTaskTimeOutCallback& cb) {
				timeout_callback_ = cb;
				timeout_callback_();
				Next();
			}
			void Stop() { timer_.cancel(); }

		   private:
			void Next() {
				timer_.expires_after(std::chrono::hours(24));
				timer_.async_wait([this](const boost::system::error_code& error) {
					if (!error) {
						if (timeout_callback_) {
							timeout_callback_();
						}
						Next();
					}
				});
			}

		   private:
			DailyTaskTimeOutCallback timeout_callback_{nullptr};
			boost::asio::steady_timer timer_;
		};
	}  // namespace engine
}  // namespace gdl

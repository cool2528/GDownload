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
			explicit DailyTaskTimer(boost::asio::io_context& io)
				: strand_(boost::asio::make_strand(io)), timer_(strand_) {}
			void Start(const DailyTaskTimeOutCallback& cb) {
				// 整体投递到 strand，首个回调与 24h 排程都在 strand 上执行（E2）
				boost::asio::post(strand_, [this, cb] {
					timeout_callback_ = cb;
					if (timeout_callback_) {
						timeout_callback_();
					}
					Next();
				});
			}
			void Stop() {
				boost::asio::post(strand_, [this] { timer_.cancel(); });
			}

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
			boost::asio::strand<boost::asio::io_context::executor_type> strand_;
			DailyTaskTimeOutCallback timeout_callback_{nullptr};
			boost::asio::steady_timer timer_;
		};
	}  // namespace engine
}  // namespace gdl

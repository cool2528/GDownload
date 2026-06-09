#pragma once
#include <algorithm>
#include <boost/asio.hpp>
#include <map>
#include <vector>
namespace gdl {
	namespace engine {
		template <typename T>
		class PubSubSystem {
		   public:
			struct Subscription {
				std::function<void(const T&)> handler;
				bool active = true;
			};

		   private:
			boost::asio::io_context& io_context_;
			boost::asio::io_context::strand strand_;
			std::map<std::string, std::vector<std::shared_ptr<Subscription>>> subscribers_;

		   public:
			explicit PubSubSystem(boost::asio::io_context& io_context) : io_context_(io_context), strand_(io_context) {}
			~PubSubSystem() {}
			void Publish(const std::string& topic, T message) {
				boost::asio::post(strand_, [this, topic, message = std::move(message)]() {
					auto it = subscribers_.find(topic);
					if (it != subscribers_.end()) {
						for (auto& sub : it->second) {
							if (sub->active) {
								boost::asio::post(io_context_, [sub, message]() { sub->handler(message); });
							}
						}
					}
				});
			}

			std::shared_ptr<Subscription> Subscribe(const std::string& topic, std::function<void(const T&)> handler) {
				auto subscription	  = std::make_shared<Subscription>();
				subscription->handler = std::move(handler);

				boost::asio::post(strand_,
								  [this, topic, subscription]() { subscribers_[topic].push_back(subscription); });

				return subscription;
			}

			void Unsubscribe(std::shared_ptr<Subscription> subscription) {
				boost::asio::post(strand_, [this, subscription]() {
					if (!subscription) {
						return;
					}
					subscription->active = false;
					for (auto& [_, subscribers] : subscribers_) {
						subscribers.erase(std::remove_if(subscribers.begin(), subscribers.end(),
														 [](const auto& sub) { return !sub || !sub->active; }),
										  subscribers.end());
					}
				});
			}
		};
	}  // namespace engine

}  // namespace gdl

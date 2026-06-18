#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include "publish_subscribe_system.h"

namespace {

TEST(PubSubSystemTest, UnsubscribeMarksSubscriptionInactiveImmediately) {
    boost::asio::io_context io;
    gdl::engine::PubSubSystem<int> pubsub(io);

    auto subscription = pubsub.Subscribe("topic", [](const int&) {});

    pubsub.Unsubscribe(subscription);

    EXPECT_FALSE(subscription->active.load());
}

}  // namespace

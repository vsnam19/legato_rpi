#include <gtest/gtest.h>
#include "eventDispatcher.hpp"
#include <thread>
#include <vector>
#include <atomic>

using namespace cfg::events;

class EventDispatcherTest : public ::testing::Test {
protected:
    EventDispatcher dispatcher_;
};

TEST_F(EventDispatcherTest, SubscribeAndReceiveNotification) {
    uint32_t targetId = 0x01020005;
    std::string receivedVal;
    uint32_t receivedId = 0;
    int callCount = 0;

    auto token = dispatcher_.Subscribe(targetId, [&](uint32_t id, std::string_view val) {
        receivedId = id;
        receivedVal = std::string(val);
        callCount++;
    });

    EXPECT_GT(token, 0);
    EXPECT_EQ(dispatcher_.GetSubscriberCount(targetId), 1);

    // Notify targetId
    dispatcher_.NotifyChange(targetId, "NewValue123");

    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(receivedId, targetId);
    EXPECT_EQ(receivedVal, "NewValue123");
}

TEST_F(EventDispatcherTest, IsolationBetweenDifferentCfgIds) {
    uint32_t id1 = 0x01020001;
    uint32_t id2 = 0x01020002;

    int count1 = 0;
    int count2 = 0;

    dispatcher_.Subscribe(id1, [&](uint32_t, std::string_view) { count1++; });
    dispatcher_.Subscribe(id2, [&](uint32_t, std::string_view) { count2++; });

    // Notify only id1
    dispatcher_.NotifyChange(id1, "Val1");
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 0);

    // Notify only id2
    dispatcher_.NotifyChange(id2, "Val2");
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
}

TEST_F(EventDispatcherTest, UnsubscribeStopsEvents) {
    uint32_t targetId = 0x0102000A;
    int callCount = 0;

    auto token = dispatcher_.Subscribe(targetId, [&](uint32_t, std::string_view) {
        callCount++;
    });

    dispatcher_.NotifyChange(targetId, "First");
    EXPECT_EQ(callCount, 1);

    // Unsubscribe
    EXPECT_TRUE(dispatcher_.Unsubscribe(token));
    EXPECT_EQ(dispatcher_.GetSubscriberCount(targetId), 0);

    // Second notification should not be delivered
    dispatcher_.NotifyChange(targetId, "Second");
    EXPECT_EQ(callCount, 1);

    // Unsubscribing again returns false
    EXPECT_FALSE(dispatcher_.Unsubscribe(token));
}

TEST_F(EventDispatcherTest, MultipleSubscribersOnSameId) {
    uint32_t targetId = 0x010200FF;
    int countA = 0;
    int countB = 0;

    auto tokenA = dispatcher_.Subscribe(targetId, [&](uint32_t, std::string_view) { countA++; });
    auto tokenB = dispatcher_.Subscribe(targetId, [&](uint32_t, std::string_view) { countB++; });

    EXPECT_EQ(dispatcher_.GetSubscriberCount(targetId), 2);

    dispatcher_.NotifyChange(targetId, "Broadcast");
    EXPECT_EQ(countA, 1);
    EXPECT_EQ(countB, 1);

    // Unsubscribe A only
    EXPECT_TRUE(dispatcher_.Unsubscribe(tokenA));
    EXPECT_EQ(dispatcher_.GetSubscriberCount(targetId), 1);

    dispatcher_.NotifyChange(targetId, "Broadcast2");
    EXPECT_EQ(countA, 1);
    EXPECT_EQ(countB, 2);

    EXPECT_TRUE(dispatcher_.Unsubscribe(tokenB));
    EXPECT_EQ(dispatcher_.GetSubscriberCount(targetId), 0);
    EXPECT_EQ(dispatcher_.GetTotalSubscribers(), 0);
}

TEST_F(EventDispatcherTest, NullCallbackRejection) {
    EXPECT_EQ(dispatcher_.Subscribe(0x12345, nullptr), 0);
    EXPECT_EQ(dispatcher_.GetTotalSubscribers(), 0);
}

TEST_F(EventDispatcherTest, ThreadSafeConcurrentNotifications) {
    uint32_t id = 0x01020011;
    std::atomic<int> counter{0};

    dispatcher_.Subscribe(id, [&](uint32_t, std::string_view) {
        counter++;
    });

    constexpr int kThreads = 8;
    constexpr int kPerThread = 100;
    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&]() {
            for (int i = 0; i < kPerThread; ++i) {
                dispatcher_.NotifyChange(id, "Concurrent");
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    EXPECT_EQ(counter.load(), kThreads * kPerThread);
}

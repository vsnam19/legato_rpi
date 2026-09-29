#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <mutex>
#include <unordered_map>

namespace cfg::events {

using ChangeCallback = std::function<void(uint32_t cfgId, std::string_view valueStr)>;
using SubscriptionToken = uint64_t;

class EventDispatcher {
public:
    EventDispatcher() = default;
    ~EventDispatcher() = default;

    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;

    SubscriptionToken Subscribe(uint32_t cfgId, ChangeCallback callback);
    bool Unsubscribe(SubscriptionToken token);
    void NotifyChange(uint32_t cfgId, std::string_view valueStr);

    [[nodiscard]] size_t GetSubscriberCount(uint32_t cfgId) const;
    [[nodiscard]] size_t GetTotalSubscribers() const;

private:
    struct Subscriber {
        SubscriptionToken token;
        uint32_t cfgId;
        ChangeCallback callback;
    };

    mutable std::mutex mutex_;
    SubscriptionToken nextToken_{1};
    std::unordered_map<SubscriptionToken, Subscriber> subscribersByToken_;
    std::unordered_multimap<uint32_t, SubscriptionToken> tokensByCfgId_;
};

} // namespace cfg::events

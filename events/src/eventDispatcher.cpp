#include "eventDispatcher.hpp"
#include <vector>

namespace cfg::events {

SubscriptionToken EventDispatcher::Subscribe(uint32_t cfgId, ChangeCallback callback) {
    if (!callback) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    SubscriptionToken token = nextToken_++;

    subscribersByToken_.emplace(token, Subscriber{token, cfgId, std::move(callback)});
    tokensByCfgId_.emplace(cfgId, token);

    return token;
}

bool EventDispatcher::Unsubscribe(SubscriptionToken token) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = subscribersByToken_.find(token);
    if (it == subscribersByToken_.end()) {
        return false;
    }

    uint32_t cfgId = it->second.cfgId;
    subscribersByToken_.erase(it);

    auto range = tokensByCfgId_.equal_range(cfgId);
    for (auto mapIt = range.first; mapIt != range.second; ++mapIt) {
        if (mapIt->second == token) {
            tokensByCfgId_.erase(mapIt);
            break;
        }
    }

    return true;
}

void EventDispatcher::NotifyChange(uint32_t cfgId, std::string_view valueStr) {
    std::vector<ChangeCallback> callbacksToInvoke;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto range = tokensByCfgId_.equal_range(cfgId);
        for (auto it = range.first; it != range.second; ++it) {
            auto subIt = subscribersByToken_.find(it->second);
            if (subIt != subscribersByToken_.end() && subIt->second.callback) {
                callbacksToInvoke.push_back(subIt->second.callback);
            }
        }
    }

    // Invoke callbacks outside the lock to prevent deadlock if a callback calls Subscribe/Unsubscribe
    for (const auto& cb : callbacksToInvoke) {
        cb(cfgId, valueStr);
    }
}

size_t EventDispatcher::GetSubscriberCount(uint32_t cfgId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tokensByCfgId_.count(cfgId);
}

size_t EventDispatcher::GetTotalSubscribers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return subscribersByToken_.size();
}

} // namespace cfg::events

#pragma once

#include "cfgId.hpp"
#include "cfgRouter.hpp"
#include "IConfigBackend.hpp"
#include "cryptoEngine.hpp"
#include "ISecurityProvider.hpp"
#include "eventDispatcher.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <span>
#include <unordered_set>
#include <mutex>
#include <vector>

#if __has_include("legato.h")
#include "legato.h"
#else
typedef enum {
    LE_OK = 0,
    LE_NOT_FOUND = -1,
    LE_NOT_PERMITTED = -5,
    LE_FAULT = -6,
    LE_OVERFLOW = -9,
    LE_BAD_PARAMETER = -15
} le_result_t;
#endif

namespace cfg::service {

/**
 * @brief Core service engine coordinating routing, storage backends, encryption, and notifications.
 */
class CfgManagerService {
public:
    CfgManagerService(
        std::unique_ptr<cfg::CfgRouter> router,
        std::unique_ptr<cfgmanager::storage::IConfigBackend> storage,
        std::unique_ptr<cfg::crypto::CryptoEngine> crypto,
        std::unique_ptr<cfg::security::ISecurityProvider> security,
        std::unique_ptr<cfg::events::EventDispatcher> events
    );
    ~CfgManagerService() = default;

    /**
     * @brief Initialize master key and backend dependencies.
     */
    le_result_t Initialize();

    le_result_t SetString(uint32_t cfgId, std::string_view value);
    le_result_t GetString(uint32_t cfgId, char* outBuffer, size_t maxLen) const;

    le_result_t SetBinary(uint32_t cfgId, std::span<const uint8_t> data);
    le_result_t GetBinary(uint32_t cfgId, uint8_t* outBuffer, size_t& inOutLen) const;

    le_result_t Delete(uint32_t cfgId);

    cfg::events::SubscriptionToken Subscribe(uint32_t cfgId, cfg::events::ChangeCallback cb);
    bool Unsubscribe(cfg::events::SubscriptionToken token);

    [[nodiscard]] const std::vector<uint8_t>& GetMasterKey() const noexcept { return masterKey_; }

private:
    std::unique_ptr<cfg::CfgRouter> router_;
    std::unique_ptr<cfgmanager::storage::IConfigBackend> storage_;
    std::unique_ptr<cfg::crypto::CryptoEngine> crypto_;
    std::unique_ptr<cfg::security::ISecurityProvider> security_;
    std::unique_ptr<cfg::events::EventDispatcher> events_;

    std::vector<uint8_t> masterKey_;
    mutable std::mutex readOnlyMutex_;
    std::unordered_set<uint32_t> writtenReadOnlyIds_;
};

} // namespace cfg::service

namespace cfgmanager::service {
    using CfgManagerService = cfg::service::CfgManagerService;
}

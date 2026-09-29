#include "cfgManagerService.hpp"
#include <cstring>
#include <algorithm>

namespace cfg::service {

CfgManagerService::CfgManagerService(
    std::unique_ptr<cfg::CfgRouter> router,
    std::unique_ptr<cfgmanager::storage::IConfigBackend> storage,
    std::unique_ptr<cfg::crypto::CryptoEngine> crypto,
    std::unique_ptr<cfg::security::ISecurityProvider> security,
    std::unique_ptr<cfg::events::EventDispatcher> events
) : router_(std::move(router)),
    storage_(std::move(storage)),
    crypto_(std::move(crypto)),
    security_(std::move(security)),
    events_(std::move(events)) {}

le_result_t CfgManagerService::Initialize() {
    if (!security_ || !crypto_) {
        return LE_FAULT;
    }
    auto keyOpt = security_->GetMasterKey();
    if (!keyOpt.has_value() || keyOpt->empty()) {
        return LE_FAULT;
    }
    masterKey_ = *keyOpt;
    if (!crypto_->SetKey(masterKey_)) {
        return LE_FAULT;
    }
    return LE_OK;
}

le_result_t CfgManagerService::SetString(uint32_t cfgId, std::string_view value) {
    if (!cfg::ValidateId(cfgId)) {
        return LE_BAD_PARAMETER;
    }

    if (cfg::IsReadOnly(cfgId)) {
        std::lock_guard<std::mutex> lock(readOnlyMutex_);
        if (writtenReadOnlyIds_.contains(cfgId)) {
            return LE_NOT_PERMITTED;
        }
    }

    auto routeOpt = router_->ResolveRoute(cfgId);
    if (!routeOpt.has_value()) {
        return LE_BAD_PARAMETER;
    }
    const auto& route = *routeOpt;
    std::string fullPath = route.GetFullConfigTreePath();

    if (route.category == cfg::Category::Raw) {
        if (!storage_->SetString(fullPath, value)) {
            return LE_FAULT;
        }
    } else if (route.category == cfg::Category::Sensitive) {
        auto encOpt = crypto_->EncryptString(value);
        if (!encOpt.has_value()) {
            return LE_FAULT;
        }
        if (!storage_->SetString(fullPath, *encOpt)) {
            return LE_FAULT;
        }
    } else if (route.category == cfg::Category::Secure) {
        std::span<const uint8_t> bytes(reinterpret_cast<const uint8_t*>(value.data()), value.size());
        if (!security_->WriteSecure(fullPath, bytes)) {
            return LE_FAULT;
        }
    } else {
        return LE_BAD_PARAMETER;
    }

    if (cfg::IsReadOnly(cfgId)) {
        std::lock_guard<std::mutex> lock(readOnlyMutex_);
        writtenReadOnlyIds_.insert(cfgId);
    }

    if (events_) {
        events_->NotifyChange(cfgId, value);
    }

    return LE_OK;
}

le_result_t CfgManagerService::GetString(uint32_t cfgId, char* outBuffer, size_t maxLen) const {
    if (!cfg::ValidateId(cfgId) || outBuffer == nullptr || maxLen == 0) {
        return LE_BAD_PARAMETER;
    }

    auto routeOpt = router_->ResolveRoute(cfgId);
    if (!routeOpt.has_value()) {
        return LE_BAD_PARAMETER;
    }
    const auto& route = *routeOpt;
    std::string fullPath = route.GetFullConfigTreePath();

    if (route.category == cfg::Category::Raw) {
        auto valOpt = storage_->GetString(fullPath);
        if (!valOpt.has_value()) {
            return LE_NOT_FOUND;
        }
        if (valOpt->size() >= maxLen) {
            return LE_OVERFLOW;
        }
        std::memcpy(outBuffer, valOpt->data(), valOpt->size());
        outBuffer[valOpt->size()] = '\0';
        return LE_OK;
    } else if (route.category == cfg::Category::Sensitive) {
        auto cipherOpt = storage_->GetString(fullPath);
        if (!cipherOpt.has_value()) {
            return LE_NOT_FOUND;
        }
        auto plainOpt = crypto_->DecryptString(*cipherOpt);
        if (!plainOpt.has_value()) {
            return LE_FAULT;
        }
        if (plainOpt->size() >= maxLen) {
            return LE_OVERFLOW;
        }
        std::memcpy(outBuffer, plainOpt->data(), plainOpt->size());
        outBuffer[plainOpt->size()] = '\0';
        return LE_OK;
    } else if (route.category == cfg::Category::Secure) {
        auto secOpt = security_->ReadSecure(fullPath);
        if (!secOpt.has_value()) {
            return LE_NOT_FOUND;
        }
        if (secOpt->size() >= maxLen) {
            return LE_OVERFLOW;
        }
        std::memcpy(outBuffer, secOpt->data(), secOpt->size());
        outBuffer[secOpt->size()] = '\0';
        return LE_OK;
    }

    return LE_BAD_PARAMETER;
}

le_result_t CfgManagerService::SetBinary(uint32_t cfgId, std::span<const uint8_t> data) {
    if (!cfg::ValidateId(cfgId)) {
        return LE_BAD_PARAMETER;
    }

    if (cfg::IsReadOnly(cfgId)) {
        std::lock_guard<std::mutex> lock(readOnlyMutex_);
        if (writtenReadOnlyIds_.contains(cfgId)) {
            return LE_NOT_PERMITTED;
        }
    }

    auto routeOpt = router_->ResolveRoute(cfgId);
    if (!routeOpt.has_value()) {
        return LE_BAD_PARAMETER;
    }
    const auto& route = *routeOpt;
    std::string fullPath = route.GetFullConfigTreePath();

    if (route.category == cfg::Category::Raw) {
        if (!storage_->SetBinary(fullPath, data)) {
            return LE_FAULT;
        }
    } else if (route.category == cfg::Category::Sensitive) {
        auto encOpt = crypto_->Encrypt(data);
        if (!encOpt.has_value()) {
            return LE_FAULT;
        }
        if (!storage_->SetBinary(fullPath, *encOpt)) {
            return LE_FAULT;
        }
    } else if (route.category == cfg::Category::Secure) {
        if (!security_->WriteSecure(fullPath, data)) {
            return LE_FAULT;
        }
    } else {
        return LE_BAD_PARAMETER;
    }

    if (cfg::IsReadOnly(cfgId)) {
        std::lock_guard<std::mutex> lock(readOnlyMutex_);
        writtenReadOnlyIds_.insert(cfgId);
    }

    if (events_) {
        events_->NotifyChange(cfgId, "<binary data>");
    }

    return LE_OK;
}

le_result_t CfgManagerService::GetBinary(uint32_t cfgId, uint8_t* outBuffer, size_t& inOutLen) const {
    if (!cfg::ValidateId(cfgId) || outBuffer == nullptr) {
        return LE_BAD_PARAMETER;
    }

    auto routeOpt = router_->ResolveRoute(cfgId);
    if (!routeOpt.has_value()) {
        return LE_BAD_PARAMETER;
    }
    const auto& route = *routeOpt;
    std::string fullPath = route.GetFullConfigTreePath();

    if (route.category == cfg::Category::Raw) {
        auto valOpt = storage_->GetBinary(fullPath);
        if (!valOpt.has_value()) {
            return LE_NOT_FOUND;
        }
        if (valOpt->size() > inOutLen) {
            return LE_OVERFLOW;
        }
        std::memcpy(outBuffer, valOpt->data(), valOpt->size());
        inOutLen = valOpt->size();
        return LE_OK;
    } else if (route.category == cfg::Category::Sensitive) {
        auto cipherOpt = storage_->GetBinary(fullPath);
        if (!cipherOpt.has_value()) {
            return LE_NOT_FOUND;
        }
        auto plainOpt = crypto_->Decrypt(*cipherOpt);
        if (!plainOpt.has_value()) {
            return LE_FAULT;
        }
        if (plainOpt->size() > inOutLen) {
            return LE_OVERFLOW;
        }
        std::memcpy(outBuffer, plainOpt->data(), plainOpt->size());
        inOutLen = plainOpt->size();
        return LE_OK;
    } else if (route.category == cfg::Category::Secure) {
        auto secOpt = security_->ReadSecure(fullPath);
        if (!secOpt.has_value()) {
            return LE_NOT_FOUND;
        }
        if (secOpt->size() > inOutLen) {
            return LE_OVERFLOW;
        }
        std::memcpy(outBuffer, secOpt->data(), secOpt->size());
        inOutLen = secOpt->size();
        return LE_OK;
    }

    return LE_BAD_PARAMETER;
}

le_result_t CfgManagerService::Delete(uint32_t cfgId) {
    if (!cfg::ValidateId(cfgId)) {
        return LE_BAD_PARAMETER;
    }

    if (cfg::IsReadOnly(cfgId)) {
        std::lock_guard<std::mutex> lock(readOnlyMutex_);
        if (writtenReadOnlyIds_.contains(cfgId)) {
            return LE_NOT_PERMITTED;
        }
    }

    auto routeOpt = router_->ResolveRoute(cfgId);
    if (!routeOpt.has_value()) {
        return LE_BAD_PARAMETER;
    }
    const auto& route = *routeOpt;
    std::string fullPath = route.GetFullConfigTreePath();

    if (route.category == cfg::Category::Raw || route.category == cfg::Category::Sensitive) {
        if (!storage_->DeleteNode(fullPath)) {
            return LE_NOT_FOUND;
        }
    } else if (route.category == cfg::Category::Secure) {
        if (!security_->DeleteSecure(fullPath)) {
            return LE_NOT_FOUND;
        }
    } else {
        return LE_BAD_PARAMETER;
    }

    if (events_) {
        events_->NotifyChange(cfgId, "");
    }

    return LE_OK;
}

cfg::events::SubscriptionToken CfgManagerService::Subscribe(uint32_t cfgId, cfg::events::ChangeCallback cb) {
    if (!events_) {
        return 0;
    }
    return events_->Subscribe(cfgId, std::move(cb));
}

bool CfgManagerService::Unsubscribe(cfg::events::SubscriptionToken token) {
    if (!events_) {
        return false;
    }
    return events_->Unsubscribe(token);
}

} // namespace cfg::service

#include "legato.h"
#include "interfaces.h"

#include "cfgManagerService.hpp"
#include "configTreeBackend.hpp"
#include "securityProvider.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace {

std::unique_ptr<cfg::service::CfgManagerService> gService;
std::mutex gHandlerMutex;
std::unordered_map<cfgManager_ChangeHandlerRef_t, cfg::events::SubscriptionToken> gTokens;

} // namespace

extern "C" {

le_result_t cfgManager_SetString(uint32_t cfgId, const char* value) {
    if (!gService || !value) {
        return LE_BAD_PARAMETER;
    }
    return gService->SetString(cfgId, value);
}

le_result_t cfgManager_GetString(uint32_t cfgId, char* value, size_t valueSize) {
    if (!gService || !value || valueSize == 0) {
        return LE_BAD_PARAMETER;
    }
    return gService->GetString(cfgId, value, valueSize);
}

le_result_t cfgManager_SetBinary(uint32_t cfgId, const uint8_t* dataPtr, size_t dataSize) {
    if (!gService || (!dataPtr && dataSize > 0)) {
        return LE_BAD_PARAMETER;
    }
    std::span<const uint8_t> spanData(dataPtr ? dataPtr : reinterpret_cast<const uint8_t*>(""), dataSize);
    return gService->SetBinary(cfgId, spanData);
}

le_result_t cfgManager_GetBinary(uint32_t cfgId, uint8_t* dataPtr, size_t* dataSizePtr) {
    if (!gService || !dataPtr || !dataSizePtr) {
        return LE_BAD_PARAMETER;
    }
    size_t inOutLen = *dataSizePtr;
    le_result_t res = gService->GetBinary(cfgId, dataPtr, inOutLen);
    if (res == LE_OK) {
        *dataSizePtr = inOutLen;
    }
    return res;
}

le_result_t cfgManager_Delete(uint32_t cfgId) {
    if (!gService) {
        return LE_FAULT;
    }
    return gService->Delete(cfgId);
}

cfgManager_ChangeHandlerRef_t cfgManager_AddChangeHandler(
    uint32_t cfgId,
    cfgManager_ChangeHandlerFunc_t handlerPtr,
    void* contextPtr
) {
    if (!gService || !handlerPtr) {
        return nullptr;
    }

    auto token = gService->Subscribe(cfgId, [handlerPtr, contextPtr](uint32_t changedId, std::string_view val) {
        handlerPtr(changedId, std::string(val).c_str(), contextPtr);
    });

    auto handlerRef = reinterpret_cast<cfgManager_ChangeHandlerRef_t>(static_cast<uintptr_t>(token));
    {
        std::lock_guard<std::mutex> lock(gHandlerMutex);
        gTokens[handlerRef] = token;
    }
    return handlerRef;
}

void cfgManager_RemoveChangeHandler(cfgManager_ChangeHandlerRef_t handlerRef) {
    if (!gService || !handlerRef) {
        return;
    }
    cfg::events::SubscriptionToken token = 0;
    {
        std::lock_guard<std::mutex> lock(gHandlerMutex);
        auto it = gTokens.find(handlerRef);
        if (it != gTokens.end()) {
            token = it->second;
            gTokens.erase(it);
        }
    }
    if (token != 0) {
        gService->Unsubscribe(token);
    }
}

COMPONENT_INIT {
    LE_INFO("Initializing Legato CfgManager Service (C++23)...");

    auto router = std::make_unique<cfg::CfgRouter>();
    auto storage = std::make_unique<cfgmanager::storage::ConfigTreeBackend>("cfgManager");
    auto crypto = std::make_unique<cfg::crypto::CryptoEngine>();
    auto security = cfg::security::CreateSecurityProvider();
    auto events = std::make_unique<cfg::events::EventDispatcher>();

    gService = std::make_unique<cfg::service::CfgManagerService>(
        std::move(router),
        std::move(storage),
        std::move(crypto),
        std::move(security),
        std::move(events)
    );

    le_result_t initRes = gService->Initialize();
    if (initRes != LE_OK) {
        LE_FATAL("Failed to initialize CfgManagerService: %d", initRes);
        return;
    }

    cfgManager_AdvertiseService();
    LE_INFO("CfgManager service advertised and ready for IPC clients.");
}

} // extern "C"

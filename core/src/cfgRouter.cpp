#include "cfgRouter.hpp"

namespace cfg {

namespace {

std::string_view SubsystemToPrefix(Subsystem sub) noexcept {
    switch (sub) {
        case Subsystem::System:   return "sys";
        case Subsystem::Network:  return "net";
        case Subsystem::Security: return "sec";
        case Subsystem::App:      return "app";
    }
    return "unk";
}

} // namespace

std::optional<StorageRoute> CfgRouter::ResolveRoute(uint32_t cfgId) const {
    if (!ValidateId(cfgId)) {
        return std::nullopt;
    }

    StorageRoute route;
    route.category = GetCategory(cfgId);
    route.isReadOnly = IsReadOnly(cfgId);
    route.isVolatile = IsVolatile(cfgId);

    auto it = customKeyMap_.find(cfgId);
    if (it != customKeyMap_.end()) {
        route.internalKey = it->second;
    } else {
        auto sub = GetSubsystem(cfgId);
        auto item = GetItemId(cfgId);
        route.internalKey = std::string(SubsystemToPrefix(sub)) + "/item_" + std::to_string(item);
    }

    return route;
}

bool CfgRouter::RegisterCustomKey(uint32_t cfgId, std::string_view customKey) {
    if (!ValidateId(cfgId) || customKey.empty()) {
        return false;
    }
    customKeyMap_[cfgId] = std::string(customKey);
    return true;
}

} // namespace cfg

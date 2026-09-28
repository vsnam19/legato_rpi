#pragma once

#include "cfgId.hpp"
#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>

namespace cfg {

struct StorageRoute {
    Category category;
    std::string internalKey;
    bool isReadOnly{false};
    bool isVolatile{false};

    [[nodiscard]] std::string GetFullConfigTreePath() const {
        return "/cfgManager/" + internalKey;
    }
};

class CfgRouter {
public:
    CfgRouter() = default;
    ~CfgRouter() = default;

    [[nodiscard]] std::optional<StorageRoute> ResolveRoute(uint32_t cfgId) const;
    bool RegisterCustomKey(uint32_t cfgId, std::string_view customKey);

private:
    std::unordered_map<uint32_t, std::string> customKeyMap_;
};

} // namespace cfg

#pragma once

#include "ISecurityProvider.hpp"

namespace cfg::security {

std::unique_ptr<ISecurityProvider> CreateSecurityProvider(
    std::string_view storagePath = "/var/config/cfgManager/secure_world"
);

std::unique_ptr<ISecurityProvider> CreateOpteeProvider();
std::unique_ptr<ISecurityProvider> CreateSimulatedEnclaveProvider(std::string_view storagePath);

} // namespace cfg::security


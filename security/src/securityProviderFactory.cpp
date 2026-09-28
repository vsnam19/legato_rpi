#include "securityProvider.hpp"
#include <memory>
#include <iostream>

namespace cfg::security {

std::unique_ptr<ISecurityProvider> CreateSimulatedEnclaveProvider(std::string_view storagePath);
std::unique_ptr<ISecurityProvider> CreateOpteeProvider();

std::unique_ptr<ISecurityProvider> CreateSecurityProvider(std::string_view storagePath) {
    auto optee = CreateOpteeProvider();
    if (optee && optee->IsAvailable()) {
        return optee;
    }

    // Fallback to SimulatedEnclave for development and systems without OP-TEE firmware
    return CreateSimulatedEnclaveProvider(storagePath);
}

} // namespace cfg::security

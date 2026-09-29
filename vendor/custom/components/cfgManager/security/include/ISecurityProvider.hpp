#pragma once

#include <string_view>
#include <vector>
#include <span>
#include <optional>
#include <memory>
#include <cstdint>

namespace cfg::security {

enum class ProviderType {
    Optee,
    SimulatedEnclave
};

class ISecurityProvider {
public:
    virtual ~ISecurityProvider() = default;

    [[nodiscard]] virtual ProviderType GetType() const noexcept = 0;
    [[nodiscard]] virtual bool IsAvailable() const noexcept = 0;

    // Master Key (256-bit / 32 bytes) used for envelope encryption of SensitiveData
    [[nodiscard]] virtual std::optional<std::vector<uint8_t>> GetMasterKey() = 0;

    // Secure Storage operations for SECURE data
    virtual bool WriteSecure(std::string_view key, std::span<const uint8_t> data) = 0;
    [[nodiscard]] virtual std::optional<std::vector<uint8_t>> ReadSecure(std::string_view key) = 0;
    virtual bool DeleteSecure(std::string_view key) = 0;
};

} // namespace cfg::security

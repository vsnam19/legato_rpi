#pragma once

#include <cstdint>

namespace cfg {

enum class Category : uint8_t {
    Raw       = 0x01,
    Sensitive = 0x02,
    Secure    = 0x04
};

enum class Subsystem : uint8_t {
    System   = 0x01,
    Network  = 0x02,
    Security = 0x03,
    App      = 0x04
};

namespace Flag {
    inline constexpr uint8_t None     = 0x00;
    inline constexpr uint8_t ReadOnly = 0x10;
    inline constexpr uint8_t Volatile = 0x20;
}

constexpr uint32_t MakeId(Category cat, Subsystem sub, uint16_t item, uint8_t extraFlags = 0) noexcept {
    return (static_cast<uint32_t>(static_cast<uint8_t>(cat) | extraFlags) << 24) |
           (static_cast<uint32_t>(static_cast<uint8_t>(sub)) << 16) |
           (static_cast<uint32_t>(item));
}

constexpr Category GetCategory(uint32_t cfgId) noexcept {
    return static_cast<Category>((cfgId >> 24) & 0x07);
}

constexpr Subsystem GetSubsystem(uint32_t cfgId) noexcept {
    return static_cast<Subsystem>((cfgId >> 16) & 0xFF);
}

constexpr uint16_t GetItemId(uint32_t cfgId) noexcept {
    return static_cast<uint16_t>(cfgId & 0xFFFF);
}

constexpr bool IsReadOnly(uint32_t cfgId) noexcept {
    return ((cfgId >> 24) & Flag::ReadOnly) != 0;
}

constexpr bool IsVolatile(uint32_t cfgId) noexcept {
    return ((cfgId >> 24) & Flag::Volatile) != 0;
}

constexpr bool ValidateId(uint32_t cfgId) noexcept {
    uint8_t flags = static_cast<uint8_t>((cfgId >> 24) & 0xFF);
    uint8_t subsys = static_cast<uint8_t>((cfgId >> 16) & 0xFF);

    // Check for invalid/unsupported flag bits (only bits 0, 1, 2, 4, 5 allowed)
    constexpr uint8_t allowedFlags = 0x01 | 0x02 | 0x04 | Flag::ReadOnly | Flag::Volatile;
    if ((flags & ~allowedFlags) != 0) {
        return false;
    }

    // Check category: exactly one category bit must be set (Raw, Sensitive, or Secure)
    uint8_t catBits = flags & 0x07;
    if (catBits != static_cast<uint8_t>(Category::Raw) &&
        catBits != static_cast<uint8_t>(Category::Sensitive) &&
        catBits != static_cast<uint8_t>(Category::Secure)) {
        return false;
    }

    // Check subsystem: must be in range [1..4]
    if (subsys < static_cast<uint8_t>(Subsystem::System) ||
        subsys > static_cast<uint8_t>(Subsystem::App)) {
        return false;
    }

    return true;
}

} // namespace cfg

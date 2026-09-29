#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <span>
#include <optional>
#include <cstdint>

namespace cfg::crypto {

class CryptoEngine {
public:
    CryptoEngine() = default;
    explicit CryptoEngine(std::span<const uint8_t> key256);
    ~CryptoEngine() = default;

    bool SetKey(std::span<const uint8_t> key256);
    [[nodiscard]] bool HasValidKey() const noexcept;

    // Binary Encrypt / Decrypt
    [[nodiscard]] std::optional<std::vector<uint8_t>> Encrypt(std::span<const uint8_t> plaintext) const;
    [[nodiscard]] std::optional<std::vector<uint8_t>> Decrypt(std::span<const uint8_t> envelope) const;

    // String Encrypt / Decrypt (returns / accepts Base64 representation of envelope)
    [[nodiscard]] std::optional<std::string> EncryptString(std::string_view plaintext) const;
    [[nodiscard]] std::optional<std::string> DecryptString(std::string_view base64Envelope) const;

    // Base64 utilities
    [[nodiscard]] static std::string Base64Encode(std::span<const uint8_t> data);
    [[nodiscard]] static std::optional<std::vector<uint8_t>> Base64Decode(std::string_view b64);

private:
    std::vector<uint8_t> key_;
};

} // namespace cfg::crypto

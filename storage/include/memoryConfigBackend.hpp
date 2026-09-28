#pragma once

#include "IConfigBackend.hpp"
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>

namespace cfgmanager::storage {

/**
 * @brief Thread-safe in-memory configuration backend implementation.
 */
class MemoryConfigBackend : public IConfigBackend {
public:
    MemoryConfigBackend() = default;
    ~MemoryConfigBackend() override = default;

    bool SetString(std::string_view path, std::string_view value) override;
    std::optional<std::string> GetString(std::string_view path) const override;

    bool SetBinary(std::string_view path, std::span<const uint8_t> data) override;
    std::optional<std::vector<uint8_t>> GetBinary(std::string_view path) const override;

    bool DeleteNode(std::string_view path) override;
    bool NodeExists(std::string_view path) const override;

    void Clear();
    [[nodiscard]] size_t Size() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> strings_;
    std::unordered_map<std::string, std::vector<uint8_t>> binaries_;
};

} // namespace cfgmanager::storage

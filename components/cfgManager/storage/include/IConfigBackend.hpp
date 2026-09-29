#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <span>
#include <cstdint>

namespace cfgmanager::storage {

/**
 * @brief Abstract interface for configuration backend storage.
 */
class IConfigBackend {
public:
    virtual ~IConfigBackend() = default;

    /**
     * @brief Set a string value at the specified canonical path.
     */
    virtual bool SetString(std::string_view path, std::string_view value) = 0;

    /**
     * @brief Get a string value from the specified canonical path.
     */
    virtual std::optional<std::string> GetString(std::string_view path) const = 0;

    /**
     * @brief Set a binary value at the specified canonical path.
     */
    virtual bool SetBinary(std::string_view path, std::span<const uint8_t> data) = 0;

    /**
     * @brief Get a binary value from the specified canonical path.
     */
    virtual std::optional<std::vector<uint8_t>> GetBinary(std::string_view path) const = 0;

    /**
     * @brief Delete a configuration node at the specified canonical path.
     */
    virtual bool DeleteNode(std::string_view path) = 0;

    /**
     * @brief Check whether a configuration node exists.
     */
    virtual bool NodeExists(std::string_view path) const = 0;
};

} // namespace cfgmanager::storage

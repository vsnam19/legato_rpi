#pragma once

#include "IConfigBackend.hpp"
#include <string>
#include <memory>
#include <mutex>

namespace cfgmanager::storage {

/**
 * @brief Legato configTree adapter implementing IConfigBackend.
 * Bridges CfgManager to Legato le_cfg service with automatic fallback for host testing.
 */
class ConfigTreeBackend : public IConfigBackend {
public:
    explicit ConfigTreeBackend(std::string treeName = "cfgManager");
    ~ConfigTreeBackend() override;

    bool SetString(std::string_view path, std::string_view value) override;
    std::optional<std::string> GetString(std::string_view path) const override;

    bool SetBinary(std::string_view path, std::span<const uint8_t> data) override;
    std::optional<std::vector<uint8_t>> GetBinary(std::string_view path) const override;

    bool DeleteNode(std::string_view path) override;
    bool NodeExists(std::string_view path) const override;

    [[nodiscard]] const std::string& GetTreeName() const noexcept { return treeName_; }

private:
    std::string treeName_;
    mutable std::mutex mutex_;
    std::unique_ptr<IConfigBackend> mockBackend_;
    bool useLegatoRuntime_{false};
};

} // namespace cfgmanager::storage

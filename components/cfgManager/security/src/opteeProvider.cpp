#include "ISecurityProvider.hpp"
#include <fcntl.h>
#include <unistd.h>

namespace cfg::security {

class OpteeProvider : public ISecurityProvider {
public:
    OpteeProvider() {
        CheckTeeDevice();
    }

    ProviderType GetType() const noexcept override {
        return ProviderType::Optee;
    }

    bool IsAvailable() const noexcept override {
        return isAvailable_;
    }

    std::optional<std::vector<uint8_t>> GetMasterKey() override {
        if (!isAvailable_) {
            return std::nullopt;
        }
        // In full OP-TEE deployment, this derives a hardware key via PTA_SYSTEM
        return std::nullopt;
    }

    bool WriteSecure(std::string_view key, std::span<const uint8_t> data) override {
        if (!isAvailable_ || key.empty()) {
            return false;
        }
        // In full OP-TEE deployment, this writes to TEE_CreatePersistentObject
        return false;
    }

    std::optional<std::vector<uint8_t>> ReadSecure(std::string_view key) override {
        if (!isAvailable_ || key.empty()) {
            return std::nullopt;
        }
        return std::nullopt;
    }

    bool DeleteSecure(std::string_view key) override {
        if (!isAvailable_ || key.empty()) {
            return false;
        }
        return false;
    }

private:
    bool isAvailable_{false};

    void CheckTeeDevice() {
        int fd = open("/dev/tee0", O_RDWR);
        if (fd >= 0) {
            isAvailable_ = true;
            close(fd);
        } else {
            isAvailable_ = false;
        }
    }
};

std::unique_ptr<ISecurityProvider> CreateOpteeProvider() {
    return std::make_unique<OpteeProvider>();
}

} // namespace cfg::security

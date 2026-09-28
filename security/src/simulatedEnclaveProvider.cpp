#include "ISecurityProvider.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace cfg::security {

namespace {

std::string SanitizeKey(std::string_view key) {
    std::string safeKey;
    safeKey.reserve(key.size());
    for (char c : key) {
        if (c == '/' || c == '\\' || c == ':' || c == '.') {
            safeKey += '_';
        } else {
            safeKey += c;
        }
    }
    return safeKey;
}

} // namespace

class SimulatedEnclaveProvider : public ISecurityProvider {
public:
    explicit SimulatedEnclaveProvider(std::string_view storagePath)
        : storagePath_(storagePath) {
        InitDirectory();
    }

    ProviderType GetType() const noexcept override {
        return ProviderType::SimulatedEnclave;
    }

    bool IsAvailable() const noexcept override {
        return true;
    }

    std::optional<std::vector<uint8_t>> GetMasterKey() override {
        if (!masterKeyCache_.empty()) {
            return masterKeyCache_;
        }

        std::string keyFilePath = storagePath_ + "/.master.key";
        if (std::filesystem::exists(keyFilePath)) {
            std::ifstream file(keyFilePath, std::ios::binary);
            if (file) {
                std::vector<uint8_t> key(32);
                file.read(reinterpret_cast<char*>(key.data()), 32);
                if (file.gcount() == 32) {
                    masterKeyCache_ = std::move(key);
                    return masterKeyCache_;
                }
            }
        }

        // Derive key using PBKDF2 with machine-id and hardware salt
        std::string machineId = "default-rpi5-device-salt";
        std::ifstream midFile("/etc/machine-id");
        if (midFile) {
            std::string line;
            if (std::getline(midFile, line) && !line.empty()) {
                machineId = line;
            }
        }

        const char salt[] = "Legato-CfgManager-SecureWorld-Salt-2026";
        std::vector<uint8_t> derivedKey(32);
        if (PKCS5_PBKDF2_HMAC(machineId.c_str(), static_cast<int>(machineId.size()),
                              reinterpret_cast<const unsigned char*>(salt), static_cast<int>(strlen(salt)),
                              10000, EVP_sha256(), 32, derivedKey.data()) != 1) {
            return std::nullopt;
        }

        // Save master key with mode 0600
        int fd = open(keyFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd >= 0) {
            (void)fchmod(fd, S_IRUSR | S_IWUSR);
            ssize_t written = write(fd, derivedKey.data(), derivedKey.size());
            fsync(fd);
            close(fd);
            if (written == 32) {
                masterKeyCache_ = std::move(derivedKey);
                return masterKeyCache_;
            }
        }

        return std::nullopt;
    }

    bool WriteSecure(std::string_view key, std::span<const uint8_t> data) override {
        if (key.empty()) {
            return false;
        }

        std::string safeKey = SanitizeKey(key);
        std::string filePath = storagePath_ + "/" + safeKey + ".sec";
        std::string tmpPath = filePath + ".tmp";

        int fd = open(tmpPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            return false;
        }

        (void)fchmod(fd, S_IRUSR | S_IWUSR);

        if (!data.empty()) {
            ssize_t written = write(fd, data.data(), data.size());
            if (written != static_cast<ssize_t>(data.size())) {
                close(fd);
                unlink(tmpPath.c_str());
                return false;
            }
        }

        fsync(fd);
        close(fd);

        if (rename(tmpPath.c_str(), filePath.c_str()) != 0) {
            unlink(tmpPath.c_str());
            return false;
        }

        return true;
    }

    std::optional<std::vector<uint8_t>> ReadSecure(std::string_view key) override {
        if (key.empty()) {
            return std::nullopt;
        }

        std::string safeKey = SanitizeKey(key);
        std::string filePath = storagePath_ + "/" + safeKey + ".sec";

        if (!std::filesystem::exists(filePath)) {
            return std::nullopt;
        }

        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file) {
            return std::nullopt;
        }

        auto fileSize = file.tellg();
        if (fileSize < 0) {
            return std::nullopt;
        }

        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> buffer(fileSize);
        if (fileSize > 0) {
            file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
            if (file.gcount() != fileSize) {
                return std::nullopt;
            }
        }

        return buffer;
    }

    bool DeleteSecure(std::string_view key) override {
        if (key.empty()) {
            return false;
        }

        std::string safeKey = SanitizeKey(key);
        std::string filePath = storagePath_ + "/" + safeKey + ".sec";

        if (!std::filesystem::exists(filePath)) {
            return false;
        }

        return std::filesystem::remove(filePath);
    }

private:
    std::string storagePath_;
    std::vector<uint8_t> masterKeyCache_;

    void InitDirectory() {
        std::error_code ec;
        std::filesystem::create_directories(storagePath_, ec);
        chmod(storagePath_.c_str(), S_IRWXU);
    }
};

std::unique_ptr<ISecurityProvider> CreateSimulatedEnclaveProvider(std::string_view storagePath) {
    return std::make_unique<SimulatedEnclaveProvider>(storagePath);
}

} // namespace cfg::security

#include "memoryConfigBackend.hpp"

namespace cfgmanager::storage {

bool MemoryConfigBackend::SetString(std::string_view path, std::string_view value) {
    if (path.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    strings_[std::string(path)] = std::string(value);
    return true;
}

std::optional<std::string> MemoryConfigBackend::GetString(std::string_view path) const {
    if (path.empty()) {
        return std::nullopt;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = strings_.find(std::string(path));
    if (it != strings_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool MemoryConfigBackend::SetBinary(std::string_view path, std::span<const uint8_t> data) {
    if (path.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    binaries_[std::string(path)] = std::vector<uint8_t>(data.begin(), data.end());
    return true;
}

std::optional<std::vector<uint8_t>> MemoryConfigBackend::GetBinary(std::string_view path) const {
    if (path.empty()) {
        return std::nullopt;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = binaries_.find(std::string(path));
    if (it != binaries_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool MemoryConfigBackend::DeleteNode(std::string_view path) {
    if (path.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key(path);
    bool erased = false;
    if (strings_.erase(key) > 0) {
        erased = true;
    }
    if (binaries_.erase(key) > 0) {
        erased = true;
    }
    return erased;
}

bool MemoryConfigBackend::NodeExists(std::string_view path) const {
    if (path.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key(path);
    return (strings_.contains(key) || binaries_.contains(key));
}

void MemoryConfigBackend::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    strings_.clear();
    binaries_.clear();
}

size_t MemoryConfigBackend::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return strings_.size() + binaries_.size();
}

} // namespace cfgmanager::storage

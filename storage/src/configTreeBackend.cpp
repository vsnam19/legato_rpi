#include "configTreeBackend.hpp"
#include "memoryConfigBackend.hpp"
#include <iostream>

#if __has_include("legato.h")
#include "legato.h"
#include "interfaces.h"
#define HAVE_LEGATO_CONFIG_TREE 1
#else
#define HAVE_LEGATO_CONFIG_TREE 0
#endif

namespace cfgmanager::storage {

ConfigTreeBackend::ConfigTreeBackend(std::string treeName)
    : treeName_(std::move(treeName)) {
#if HAVE_LEGATO_CONFIG_TREE
    useLegatoRuntime_ = true;
#else
    useLegatoRuntime_ = false;
    mockBackend_ = std::make_unique<MemoryConfigBackend>();
#endif
}

ConfigTreeBackend::~ConfigTreeBackend() = default;

bool ConfigTreeBackend::SetString(std::string_view path, std::string_view value) {
    if (path.empty()) {
        return false;
    }

#if HAVE_LEGATO_CONFIG_TREE
    if (useLegatoRuntime_) {
        std::string fullPath = treeName_ + ":" + std::string(path);
        le_cfg_IteratorRef_t iterator = le_cfg_CreateWriteTxn(fullPath.c_str());
        le_cfg_SetString(iterator, "", std::string(value).c_str());
        le_cfg_CommitTxn(iterator);
        return true;
    }
#endif

    if (mockBackend_) {
        return mockBackend_->SetString(path, value);
    }
    return false;
}

std::optional<std::string> ConfigTreeBackend::GetString(std::string_view path) const {
    if (path.empty()) {
        return std::nullopt;
    }

#if HAVE_LEGATO_CONFIG_TREE
    if (useLegatoRuntime_) {
        std::string fullPath = treeName_ + ":" + std::string(path);
        le_cfg_IteratorRef_t iterator = le_cfg_CreateReadTxn(fullPath.c_str());
        if (!le_cfg_NodeExists(iterator, "")) {
            le_cfg_CancelTxn(iterator);
            return std::nullopt;
        }
        char buffer[4096] = {0};
        le_result_t res = le_cfg_GetString(iterator, "", buffer, sizeof(buffer), "");
        le_cfg_CancelTxn(iterator);
        if (res == LE_OK) {
            return std::string(buffer);
        }
        return std::nullopt;
    }
#endif

    if (mockBackend_) {
        return mockBackend_->GetString(path);
    }
    return std::nullopt;
}

bool ConfigTreeBackend::SetBinary(std::string_view path, std::span<const uint8_t> data) {
    if (path.empty()) {
        return false;
    }

#if HAVE_LEGATO_CONFIG_TREE
    if (useLegatoRuntime_) {
        std::string fullPath = treeName_ + ":" + std::string(path);
        le_cfg_IteratorRef_t iterator = le_cfg_CreateWriteTxn(fullPath.c_str());
        le_cfg_SetBinary(iterator, "", data.data(), data.size());
        le_cfg_CommitTxn(iterator);
        return true;
    }
#endif

    if (mockBackend_) {
        return mockBackend_->SetBinary(path, data);
    }
    return false;
}

std::optional<std::vector<uint8_t>> ConfigTreeBackend::GetBinary(std::string_view path) const {
    if (path.empty()) {
        return std::nullopt;
    }

#if HAVE_LEGATO_CONFIG_TREE
    if (useLegatoRuntime_) {
        std::string fullPath = treeName_ + ":" + std::string(path);
        le_cfg_IteratorRef_t iterator = le_cfg_CreateReadTxn(fullPath.c_str());
        if (!le_cfg_NodeExists(iterator, "")) {
            le_cfg_CancelTxn(iterator);
            return std::nullopt;
        }
        uint8_t buffer[8192] = {0};
        size_t actualSize = sizeof(buffer);
        le_result_t res = le_cfg_GetBinary(iterator, "", buffer, &actualSize, nullptr, 0);
        le_cfg_CancelTxn(iterator);
        if (res == LE_OK) {
            return std::vector<uint8_t>(buffer, buffer + actualSize);
        }
        return std::nullopt;
    }
#endif

    if (mockBackend_) {
        return mockBackend_->GetBinary(path);
    }
    return std::nullopt;
}

bool ConfigTreeBackend::DeleteNode(std::string_view path) {
    if (path.empty()) {
        return false;
    }

#if HAVE_LEGATO_CONFIG_TREE
    if (useLegatoRuntime_) {
        std::string fullPath = treeName_ + ":" + std::string(path);
        le_cfg_IteratorRef_t iterator = le_cfg_CreateWriteTxn(fullPath.c_str());
        if (!le_cfg_NodeExists(iterator, "")) {
            le_cfg_CancelTxn(iterator);
            return false;
        }
        le_cfg_DeleteNode(iterator, "");
        le_cfg_CommitTxn(iterator);
        return true;
    }
#endif

    if (mockBackend_) {
        return mockBackend_->DeleteNode(path);
    }
    return false;
}

bool ConfigTreeBackend::NodeExists(std::string_view path) const {
    if (path.empty()) {
        return false;
    }

#if HAVE_LEGATO_CONFIG_TREE
    if (useLegatoRuntime_) {
        std::string fullPath = treeName_ + ":" + std::string(path);
        le_cfg_IteratorRef_t iterator = le_cfg_CreateReadTxn(fullPath.c_str());
        bool exists = le_cfg_NodeExists(iterator, "");
        le_cfg_CancelTxn(iterator);
        return exists;
    }
#endif

    if (mockBackend_) {
        return mockBackend_->NodeExists(path);
    }
    return false;
}

} // namespace cfgmanager::storage

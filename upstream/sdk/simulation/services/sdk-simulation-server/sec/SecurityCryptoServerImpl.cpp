/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <telux/sec/CryptoManager.hpp>

#include "libs/common/Logger.hpp"
#include "libs/common/CommonUtils.hpp"

#include "SecurityCryptoServerImpl.hpp"

SecurityCryptoServerImpl::SecurityCryptoServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

SecurityCryptoServerImpl::~SecurityCryptoServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status SecurityCryptoServerImpl::Init(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request,
    ::commonStub::ErrorCodeMsg* response) {

    telux::common::ErrorCode ec;

    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, cryptoMgr_API_JSON_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", cryptoMgr_API_JSON_FILE);
            response->set_ec(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    if(!databaseJsonRoot_) {
        ec = JsonParser::readFromJsonFile(databaseJsonRoot_, cryptoMgr_DATABASE_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", cryptoMgr_DATABASE_FILE);
            response->set_ec(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    if (isServiceInitialized_) {
        response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
        return grpc::Status::OK;
    }

    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    isServiceInitialized_ = true;
    return grpc::Status::OK;
}

grpc::Status SecurityCryptoServerImpl::DeInit(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request,
    ::commonStub::ErrorCodeMsg* response) {

    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    return grpc::Status::OK;
}

grpc::Status SecurityCryptoServerImpl::GenerateKey(::grpc::ServerContext* context,
const ::securityStub::GenerateKeyRequest* request,
::securityStub::GenerateKeyResponse* response) {

    telux::common::ErrorCode ec;

    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, cryptoMgr_API_JSON_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", cryptoMgr_API_JSON_FILE);
            response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    std::string ecStr = apiConfigJsonRoot_["ICryptoManager"]["generateKey"]["error"].asString();
    if (handleAndSetCommonErrorCode(ecStr, __FUNCTION__, " failed to generate key", response)) {
        return grpc::Status::OK;
    }

    std::string keyBlobStr = databaseJsonRoot_["generateKey"]["keyBlob"].asString();
    std::string byteBlob = hex_to_bytes(keyBlobStr);

    response->set_key_blob(byteBlob);
    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

grpc::Status SecurityCryptoServerImpl::ImportKey(::grpc::ServerContext* context,
    const ::securityStub::ImportKeyRequest* request,
    ::securityStub::ImportKeyResponse* response) {
   telux::common::ErrorCode ec;

    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, cryptoMgr_API_JSON_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", cryptoMgr_API_JSON_FILE);
            response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    std::string ecStr = apiConfigJsonRoot_["ICryptoManager"]["importKey"]["error"].asString();
    if (handleAndSetCommonErrorCode(ecStr, __FUNCTION__, "failed to import key", response)) {
        // If the helper returned true, it means an error was mapped, logged, and
        // the response's error code was set. The gRPC call should now complete
        // with an "OK" status, carrying the error details in the response message.
        return grpc::Status::OK;
    }

    std::string keyBlobStr = databaseJsonRoot_["importKey"]["result"].asString();
    std::string byteBlob = hex_to_bytes(keyBlobStr);
    response->set_key_blob(byteBlob);
    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

grpc::Status SecurityCryptoServerImpl::ExportKey(::grpc::ServerContext* context,
    const ::securityStub::ExportKeyRequest* request,
    ::securityStub::ExportKeyResponse* response) {
    telux::common::ErrorCode ec;

    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, cryptoMgr_API_JSON_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", cryptoMgr_API_JSON_FILE);
            response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    std::string ecStr = apiConfigJsonRoot_["ICryptoManager"]["exportKey"]["error"].asString();
    if (handleAndSetCommonErrorCode(ecStr, __FUNCTION__, " failed to export key", response)) {
        // If the helper returned true, it means an error was mapped, logged, and
        // the response's error code was set. The gRPC call should now complete
        // with an "OK" status, carrying the error details in the response message.
        return grpc::Status::OK;
    }

    std::string keyDataStr = databaseJsonRoot_["exportKey"]["result"].asString();
    std::string byteData = hex_to_bytes(keyDataStr);
    response->set_key_data(byteData);
    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

grpc::Status SecurityCryptoServerImpl::UpgradeKey(::grpc::ServerContext* context,
    const ::securityStub::UpgradeKeyRequest* request,
    ::securityStub::UpgradeKeyResponse* response) {
    telux::common::ErrorCode ec;

    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, cryptoMgr_API_JSON_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", cryptoMgr_API_JSON_FILE);
            response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    std::string ecStr = apiConfigJsonRoot_["ICryptoManager"]["upgradeKey"]["error"].asString();
    if (handleAndSetCommonErrorCode(ecStr, __FUNCTION__, " failed to upgrade key", response)) {
        // If the helper returned true, it means an error was mapped, logged, and
        // the response's error code was set. The gRPC call should now complete
        // with an "OK" status, carrying the error details in the response message.
        return grpc::Status::OK;
    }

    if (!databaseJsonRoot_.isMember("upgradeKey")) {
        LOG(ERROR, __FUNCTION__, " upgradeKey result not found in database");
        response->set_error_code(commonStub::ErrorCode::INVALID_ARGUMENTS);
        return grpc::Status::OK;
    }

    std::string newKeyBlobStr = databaseJsonRoot_["upgradeKey"]["key"].asString();
    if (newKeyBlobStr.empty()) {
        LOG(ERROR, __FUNCTION__, " upgradeKey result is empty");
        response->set_error_code(commonStub::ErrorCode::INVALID_ARGUMENTS);
        return grpc::Status::OK;
    }

    std::string newKeyBlob = hex_to_bytes(newKeyBlobStr);
    if (newKeyBlob.empty()) {
        LOG(ERROR, __FUNCTION__, " failed to convert upgradeKey result to bytes");
        response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
        return grpc::Status::OK;
    }

    response->set_new_key_blob(newKeyBlob);
    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

grpc::Status SecurityCryptoServerImpl::SignData(::grpc::ServerContext* context,
    const ::securityStub::SignDataRequest* request,
    ::securityStub::SignDataResponse* response) {
    telux::common::ErrorCode ec;

    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, cryptoMgr_API_JSON_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", cryptoMgr_API_JSON_FILE);
            response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    std::string ecStr = apiConfigJsonRoot_["ICryptoManager"]["signData"]["error"].asString();
    if (handleAndSetCommonErrorCode(ecStr, __FUNCTION__, " failed to sign data", response)) {
        return grpc::Status::OK;
    }

    std::string signatureStr = databaseJsonRoot_["signData"]["signature"].asString();
    std::string signature = hex_to_bytes(signatureStr);
    response->set_signature(signature);
    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

grpc::Status SecurityCryptoServerImpl::VerifyData(::grpc::ServerContext* context,
    const ::securityStub::VerifyDataRequest* request,
    ::securityStub::VerifyDataResponse* response) {
    telux::common::ErrorCode ec;

    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, cryptoMgr_API_JSON_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", cryptoMgr_API_JSON_FILE);
            response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    std::string ecStr = apiConfigJsonRoot_["ICryptoManager"]["verifyData"]["error"].asString();
    if (handleAndSetCommonErrorCode(ecStr, __FUNCTION__, " failed to verify data", response)) {
        return grpc::Status::OK;
    }

    if (!databaseJsonRoot_.isMember("verifySign")) {
        LOG(ERROR, __FUNCTION__, " verifySign result not found in database");
        response->set_error_code(commonStub::ErrorCode::INVALID_ARGUMENTS);
        return grpc::Status::OK;
    }

    bool verified = databaseJsonRoot_["verifySign"]["result"].asBool();
    if (!verified) {
        LOG(ERROR, __FUNCTION__, " verification failed");
        response->set_verified(verified);
        response->set_error_code(commonStub::ErrorCode::INVALID_ARGUMENTS);
        return grpc::Status::OK;
    }

    response->set_verified(verified);
    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    return grpc::Status::OK;
}

grpc::Status SecurityCryptoServerImpl::EncryptData(::grpc::ServerContext* context,
    const ::securityStub::EncryptDataRequest* request,
    ::securityStub::EncryptDataResponse* response) {
    telux::common::ErrorCode ec;

    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, cryptoMgr_API_JSON_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", cryptoMgr_API_JSON_FILE);
            response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    std::string ecStr = apiConfigJsonRoot_["ICryptoManager"]["encryptData"]["error"].asString();
    if (handleAndSetCommonErrorCode(ecStr, __FUNCTION__, " failed to encrypt data", response)) {
        return grpc::Status::OK;
    }

    if (!databaseJsonRoot_.isMember("encryptData")) {
        LOG(ERROR, __FUNCTION__, " encryptData result not found in database");
        response->set_error_code(commonStub::ErrorCode::INVALID_ARGUMENTS);
        return grpc::Status::OK;
    }

    std::string encryptedDataStr = databaseJsonRoot_["encryptData"]["encryptData"].asString();
    if (encryptedDataStr.empty()) {
        LOG(ERROR, __FUNCTION__, " encryptData result is empty");
        response->set_error_code(commonStub::ErrorCode::INVALID_ARGUMENTS);
        return grpc::Status::OK;
    }

    std::string encryptedData = hex_to_bytes(encryptedDataStr);
    if (encryptedData.empty()) {
        LOG(ERROR, __FUNCTION__, " failed to convert encryptData result to bytes");
        response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
        return grpc::Status::OK;
    }

    response->set_encrypted_text(encryptedData);
    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

grpc::Status SecurityCryptoServerImpl::DecryptData(::grpc::ServerContext* context,
    const ::securityStub::DecryptDataRequest* request,
    ::securityStub::DecryptDataResponse* response) {
    telux::common::ErrorCode ec;

    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, cryptoMgr_API_JSON_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", cryptoMgr_API_JSON_FILE);
            response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    std::string ecStr = apiConfigJsonRoot_["ICryptoManager"]["decryptData"]["error"].asString();
    if (handleAndSetCommonErrorCode(ecStr, __FUNCTION__, " failed to decrypt data", response)) {
        return grpc::Status::OK;
    }

    if (!databaseJsonRoot_.isMember("decryptData")) {
        LOG(ERROR, __FUNCTION__, " decryptData result not found in database");
        response->set_error_code(commonStub::ErrorCode::INVALID_ARGUMENTS);
        return grpc::Status::OK;
    }

    std::string decryptedDataStr = databaseJsonRoot_["decryptData"]["decryptData"].asString();
    if (decryptedDataStr.empty()) {
        LOG(ERROR, __FUNCTION__, " decryptData result is empty");
        response->set_error_code(commonStub::ErrorCode::INVALID_ARGUMENTS);
        return grpc::Status::OK;
    }

    std::string decryptedData = hex_to_bytes(decryptedDataStr);
    if (decryptedData.empty()) {
        LOG(ERROR, __FUNCTION__, " failed to convert decryptData result to bytes");
        response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
        return grpc::Status::OK;
    }

    response->set_decrypted_text(decryptedData);
    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

//Helper function
std::string SecurityCryptoServerImpl::hex_to_bytes(const std::string& hex) {
    std::string bytes;
    // Check for even length (valid hex string must have even length)
    if (hex.length() % 2 != 0) {
        LOG(ERROR, __FUNCTION__, " invalid hex string length");
            return std::string();
    }

    bytes.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        char* end;
        long value = strtol(byteString.c_str(), &end, 16);
        // Check for conversion errors
        if (*end != '\0' || value < 0 || value > 255) {
            LOG(ERROR, __FUNCTION__, " invalid hex character in string");
            return std::string();
        }
        bytes.push_back(static_cast<char>(value));
    }
    return bytes;
}

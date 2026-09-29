/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/impl/codegen/async_unary_call.h>

#include <telux/sec/CryptoDefines.hpp>

#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

#include "CryptoManagerImpl.hpp"

namespace telux {
namespace sec {

std::unique_ptr<::securityStub::SecurityCryptoManagerService::Stub> CryptoManagerImpl::stub_;

/* OS version, OS security patch, ROT must be same as qseecomd */
CryptoManagerImpl::CryptoManagerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

CryptoManagerImpl::~CryptoManagerImpl() {
    LOG(DEBUG, __FUNCTION__);
    deinit();
}

telux::common::ErrorCode CryptoManagerImpl::deinit() {

    telux::common::ErrorCode ec;
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    ::commonStub::ErrorCodeMsg response{};

    reqStatus = stub_->DeInit(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "can't deregister with server, error code: ", static_cast<int>(ec));
        return ec;
    }

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode CryptoManagerImpl::init() {

    telux::common::ErrorCode ec;
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    ::commonStub::ErrorCodeMsg response{};

    stub_ = CommonUtils::getGrpcStub<securityStub::SecurityCryptoManagerService>();

    reqStatus = stub_->Init(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "can't register with server, error code: ", static_cast<int>(ec));
        return ec;
    }

    return telux::common::ErrorCode::SUCCESS;

}

/**
 * Generates a key using the provided crypto parameters.
 *
 * Calls the GenerateKey method on the gRPC stub and returns the result.
 */
telux::common::ErrorCode CryptoManagerImpl::generateKey(
    std::shared_ptr<ICryptoParam> cryptoParam, std::vector<uint8_t> &keyBlob) {

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::GenerateKeyRequest request{};
    ::securityStub::GenerateKeyResponse response{};

    reqStatus = stub_->GenerateKey(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    telux::common::ErrorCode ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "failed to generate key, error code: ", static_cast<int>(ec));
        return ec;
    }

    std::vector<unsigned char> keyBlobData(response.key_blob().begin(), response.key_blob().end());
    keyBlob.assign(keyBlobData.begin(), keyBlobData.end());

    return telux::common::ErrorCode::SUCCESS;
}

/**
 * Imports a key using the provided crypto parameters and key data.
 *
 * Calls the ImportKey method on the gRPC stub and returns the result.
 */
telux::common::ErrorCode CryptoManagerImpl::importKey(std::shared_ptr<ICryptoParam> cryptoParam,
    telux::sec::KeyFormat keyFmt, std::vector<uint8_t> const &keyData,
    std::vector<uint8_t> &keyBlob) {
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::ImportKeyRequest request{};
    ::securityStub::ImportKeyResponse response{};

    reqStatus = stub_->ImportKey(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    telux::common::ErrorCode ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "failed to import key, error code: ", static_cast<int>(ec));
        return ec;
    }

    std::vector<unsigned char> keyBlobData(response.key_blob().begin(), response.key_blob().end());
    keyBlob.assign(keyBlobData.begin(), keyBlobData.end());

    return telux::common::ErrorCode::SUCCESS;
}

/**
 * Exports a key using the provided crypto parameters and key blob.
 *
 * Calls the ExportKey method on the gRPC stub and returns the result.
 */
telux::common::ErrorCode CryptoManagerImpl::exportKey(telux::sec::KeyFormat keyFmt,
    std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> &keyData) {
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::ExportKeyRequest request{};
    ::securityStub::ExportKeyResponse response{};

    reqStatus = stub_->ExportKey(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    telux::common::ErrorCode ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "failed to export key, error code: ", static_cast<int>(ec));
        return ec;
    }

    std::vector<unsigned char> keyDataRes(response.key_data().begin(), response.key_data().end());
    keyData.assign(keyDataRes.begin(), keyDataRes.end());

    return telux::common::ErrorCode::SUCCESS;
}

/**
 * Upgrades a key using the provided crypto parameters and old key blob.
 *
 * Calls the UpgradeKey method on the gRPC stub and returns the result.
 */
telux::common::ErrorCode CryptoManagerImpl::upgradeKey(std::shared_ptr<ICryptoParam> cryptoParam,
    std::vector<uint8_t> const &oldKeyBlob, std::vector<uint8_t> &newKeyBlob) {
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::UpgradeKeyRequest request{};
    ::securityStub::UpgradeKeyResponse response{};

    reqStatus = stub_->UpgradeKey(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    telux::common::ErrorCode ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "failed to upgrade key, error code: ", static_cast<int>(ec));
        return ec;
    }

    std::vector<unsigned char> newKeyBlobRes(response.new_key_blob().begin(),
        response.new_key_blob().end());
    newKeyBlob.assign(newKeyBlobRes.begin(), newKeyBlobRes.end());
    return telux::common::ErrorCode::SUCCESS;
}

/**
 * Signs data using the provided crypto parameters and key blob.
 *
 * Calls the SignData method on the gRPC stub and returns the result.
 */
telux::common::ErrorCode CryptoManagerImpl::signData(std::shared_ptr<ICryptoParam> cryptoParam,
    std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> const &plainText,
    std::vector<uint8_t> &signature) {
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::SignDataRequest request{};
    ::securityStub::SignDataResponse response{};

    reqStatus = stub_->SignData(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    telux::common::ErrorCode ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "failed to sign data, error code: ", static_cast<int>(ec));
        return ec;
    }

    std::vector<unsigned char> signatureRes(response.signature().begin(),
        response.signature().end());
    signature.assign(signatureRes.begin(), signatureRes.end());

    return telux::common::ErrorCode::SUCCESS;
}

/**
 * Signs data using the provided crypto parameters and key blob.
 *
 * Calls the SignData method on the gRPC stub and returns the result.
 */
telux::common::ErrorCode CryptoManagerImpl::verifyData(std::shared_ptr<ICryptoParam> cryptoParam,
    std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> const &plainText,
    std::vector<uint8_t> const &signature) {
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::VerifyDataRequest request{};
    ::securityStub::VerifyDataResponse response{};

    reqStatus = stub_->VerifyData(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    telux::common::ErrorCode ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "failed to verify data, error code: ", static_cast<int>(ec));
        return ec;
    }

    return telux::common::ErrorCode::SUCCESS;
}

/**
 * Encrypts data using the provided crypto parameters and key blob.
 *
 * Calls the EncryptData method on the gRPC stub and returns the result.
 */
telux::common::ErrorCode CryptoManagerImpl::encryptData(std::shared_ptr<ICryptoParam> cryptoParam,
    std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> const &plainText,
    std::shared_ptr<EncryptedData> &encryptedData) {
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::EncryptDataRequest request{};
    ::securityStub::EncryptDataResponse response{};

    reqStatus = stub_->EncryptData(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    telux::common::ErrorCode ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "failed to encrypt data, error code: ", static_cast<int>(ec));
        return ec;
    }

    try {
        encryptedData = std::make_shared<EncryptedData>();
    } catch (const std::exception &e) {
        LOG(ERROR, __FUNCTION__, " can't allocate EncryptedData");
        return telux::common::ErrorCode::NO_MEMORY;
    }

    std::vector<uint8_t> encryptedTextVec(response.encrypted_text().begin(),
        response.encrypted_text().end());
    encryptedData->encryptedText.assign(encryptedTextVec.begin(), encryptedTextVec.end());

    return telux::common::ErrorCode::SUCCESS;
}

/**
 * Decrypts data using the provided crypto parameters and key blob.
 *
 * Calls the DecryptData method on the gRPC stub and returns the result.
 */
telux::common::ErrorCode CryptoManagerImpl::decryptData(std::shared_ptr<ICryptoParam> cryptoParam,
    std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> const &encryptedText,
    std::vector<uint8_t> &decryptedText) {
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::DecryptDataRequest request{};
    ::securityStub::DecryptDataResponse response{};

    reqStatus = stub_->DecryptData(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    telux::common::ErrorCode ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "failed to decrypt data, error code: ", static_cast<int>(ec));
        return ec;
    }

    std::vector<uint8_t> decryptedTextVec(response.decrypted_text().begin(),
        response.decrypted_text().end());
    decryptedText.assign(decryptedTextVec.begin(), decryptedTextVec.end());
    return telux::common::ErrorCode::SUCCESS;
}

}  // End of namespace sec
}  // End of namespace telux

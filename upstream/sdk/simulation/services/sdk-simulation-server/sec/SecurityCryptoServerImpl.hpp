/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SECURITY_CRYPTO_SERVER_IMPL_HPP
#define SECURITY_CRYPTO_SERVER_IMPL_HPP

#include <mutex>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>

#include "protos/proto-src/security_simulation.grpc.pb.h"

#include "libs/common/JsonParser.hpp"

class SecurityCryptoServerImpl : public ::securityStub::SecurityCryptoManagerService::Service {

 public:
    SecurityCryptoServerImpl();
    ~SecurityCryptoServerImpl();

    grpc::Status Init(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status DeInit(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status GenerateKey(::grpc::ServerContext* context,
        const ::securityStub::GenerateKeyRequest* request,
        ::securityStub::GenerateKeyResponse* response) override;

    grpc::Status ImportKey(::grpc::ServerContext* context,
        const ::securityStub::ImportKeyRequest* request,
        ::securityStub::ImportKeyResponse* response) override;

    grpc::Status ExportKey(
    ::grpc::ServerContext* context,
    const ::securityStub::ExportKeyRequest* request,
    ::securityStub::ExportKeyResponse* response) override;

    grpc::Status UpgradeKey(
        ::grpc::ServerContext* context,
        const ::securityStub::UpgradeKeyRequest* request,
        ::securityStub::UpgradeKeyResponse* response) override;

    grpc::Status SignData(
        ::grpc::ServerContext* context,
        const ::securityStub::SignDataRequest* request,
        ::securityStub::SignDataResponse* response) override;

    grpc::Status VerifyData(
        ::grpc::ServerContext* context,
        const ::securityStub::VerifyDataRequest* request,
        ::securityStub::VerifyDataResponse* response) override;

    grpc::Status EncryptData(
        ::grpc::ServerContext* context,
        const ::securityStub::EncryptDataRequest* request,
        ::securityStub::EncryptDataResponse* response) override;

    grpc::Status DecryptData(
        ::grpc::ServerContext* context,
        const ::securityStub::DecryptDataRequest* request,
        ::securityStub::DecryptDataResponse* response) override;

 private:
    const char * const cryptoMgr_API_JSON_FILE = "api/sec/ICryptoManager.json";
    const char * const cryptoMgr_DATABASE_FILE = "system-state/sec/ICryptoManager.json";

    Json::Value apiConfigJsonRoot_;
    Json::Value databaseJsonRoot_;
    bool isServiceInitialized_{false};

    std::string hex_to_bytes(const std::string& hex);

    template<typename ResponseType>
    bool handleAndSetCommonErrorCode(
        const std::string& ecStr,
        const char* funcName,
        const std::string& errMsg,
        ResponseType* response) {
        // ALL implementation details GO HERE
        telux::common::ErrorCode teluxEc = CommonUtils::mapErrorCode(ecStr);
        if (teluxEc != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, funcName, "%s", errMsg.c_str());
            response->set_error_code(static_cast<commonStub::ErrorCode>(teluxEc));
            return true;
        }
        return false;
    }
};

#endif // SECURITY_CRYPTO_SERVER_IMPL_HPP

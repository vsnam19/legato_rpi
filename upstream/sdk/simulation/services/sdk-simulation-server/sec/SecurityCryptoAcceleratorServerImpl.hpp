/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SECURITY_CRYPTO_ACC_SERVER_IMPL_HPP
#define SECURITY_CRYPTO_ACC_SERVER_IMPL_HPP

#include <mutex>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>

#include "protos/proto-src/security_simulation.grpc.pb.h"

#include "libs/common/JsonParser.hpp"

#include "event/EventService.hpp"
#include "event/ServerEventManager.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

class SecurityCryptoAcceleratorServerImpl :
    public securityStub::CryptoAcceleratorManagerService::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<SecurityCryptoAcceleratorServerImpl> {

 public:
    SecurityCryptoAcceleratorServerImpl();
    ~SecurityCryptoAcceleratorServerImpl();

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

    grpc::Status Init(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status DeInit(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status EccVerifyDigest(::grpc::ServerContext* context,
        const ::securityStub::EccVerificationRequest* request,
        ::securityStub::EccVerificationResponse* response) override;

    grpc::Status EcqvPointMultiplyAndAdd(::grpc::ServerContext* context,
        const ::securityStub::EcqvRequest* request,
        ::securityStub::EcqvResponse* response) override;

    grpc::Status EccPostDigestForVerification(::grpc::ServerContext* context,
        const ::securityStub::EccVerificationRequest* request,
        ::securityStub::EccPostVerificationResponse* response) override;

    grpc::Status EcqvPostDataForMultiplyAndAdd(::grpc::ServerContext* context,
        const ::securityStub::EcqvRequest* request,
        ::securityStub::EcqvPostResponse* response) override;

    grpc::Status GetAsyncResults(::grpc::ServerContext* context,
        const ::securityStub::GetAsyncResultsRequest* request,
        ::securityStub::GetAsyncResultsResponse* response) override;

 private:
    const char * const cryptoAccMgr_API_JSON_FILE = "api/sec/ICryptoAcceleratorManager.json";
    const char * const cryptoAccMgr_DATABASE_FILE =
        "system-state/sec/ICryptoAcceleratorManager.json";
    const char * const CRYPTOACC_FILTER = "cryptoAcc";

    Json::Value apiConfigJsonRoot_;
    Json::Value databaseJsonRoot_;
    uint32_t clientsCount_ = 0;
    bool isServiceInitialized_{false};
    ServerEventManager &serverEvent_;
    EventService &clientEvent_;

    std::string hex_to_bytes(const std::string& hex);

    template<typename ResponseType>
    bool handleAndSetCommonErrorCode(
        const std::string& ecStr,
        const char* funcName,
        const std::string& errMsg,
        ResponseType* response) {
        telux::common::ErrorCode teluxEc = CommonUtils::mapErrorCode(ecStr);
        if (teluxEc != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, funcName, "%s", errMsg.c_str());
            response->set_error_code(static_cast<commonStub::ErrorCode>(teluxEc));
            return true;
        }
        return false;
    }

    struct ApiResponse{
        int cbDelay;
        telux::common::ErrorCode error;
        telux::common::Status status;
    };

    struct AsyncOperationResult {
        uint32_t id;
        securityStub::OperationType operation_type;
        std::string result_data;
        telux::common::ErrorCode error_code;
    };

    // Members for asynchronous results management
    std::vector<AsyncOperationResult> asyncResultsQueue_;
    std::mutex asyncResultsMutex_;

    void handleSSREvent(std::string eventParams);
    std::string mapCurveToString(securityStub::EccCurve curve);
};

#endif // SECURITY_CRYPTO_ACC_SERVER_IMPL_HPP

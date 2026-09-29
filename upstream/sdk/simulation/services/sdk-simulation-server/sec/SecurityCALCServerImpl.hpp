/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SECURITY_CALC_SERVER_IMPL_HPP
#define SECURITY_CALC_SERVER_IMPL_HPP

#include <random>
#include <mutex>

#include <grpcpp/grpcpp.h>
#include "protos/proto-src/security_simulation.grpc.pb.h"
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "libs/common/JsonParser.hpp"

#include "event/EventService.hpp"
#include "event/ServerEventManager.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

class SecurityCALCServerImpl : public ::securityStub::SecurityCALCService::Service,
                               public IServerEventListener,
                               public std::enable_shared_from_this<SecurityCALCServerImpl> {

 public:
    struct LoadCount {
        uint32_t sm2;
        uint32_t nist256;
        uint32_t nist384;
        uint32_t bp256;
        uint32_t bp384;
    };
    struct CalculationCapacity {
        uint32_t sm2;
        uint32_t nist256;
        uint32_t nist384;
        uint32_t bp256;
        uint32_t bp384;
    };

    SecurityCALCServerImpl();
    ~SecurityCALCServerImpl();

    grpc::Status Init(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status DeInit(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status RegisterClient(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status DeregisterClient(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status GetCapacity(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::securityStub::Capacity* response) override;

    grpc::Status GetOperationsCount(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::securityStub::LoadCount* response) override;

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

 private:
    const uint32_t NISTP384_MAX_CAPACITY = 1992;
    const uint32_t BP384_MAX_CAPACITY = 1004;
    const uint32_t COMMON_MAX_CAPACITY = 3000;
    const char * const CALC_FILTER = "calc";
    const char * const CALC_DEFAULT_DELIMITER = " ";
    const char * const CALC_API_JSON_FILE = "api/sec/ICAControlManager.json";

    uint32_t clientsCount_ = 0;
    bool isServiceInitialized_{false};
    Json::Value apiConfigJsonRoot_;
    std::mutex operationGuard_;

    ServerEventManager &serverEvent_;
    EventService &clientEvent_;

    bool haveInjectedLoad_{false};
    LoadCount injectedLoadCount_{};
    LoadCount curLoadCount_{};
    CalculationCapacity curCapacity_{};

    std::mt19937 randRing_;
    std::uniform_int_distribution<uint32_t> randUniDistributionCommon_;
    std::uniform_int_distribution<uint32_t> randUniDistributionNist384_;
    std::uniform_int_distribution<uint32_t> randUniDistributionBp384_;

    void handleLoadEvent(std::string eventParams);
    void handleCapacityEvent(std::string eventParams);
};

#endif // SECURITY_CALC_SERVER_IMPL_HPP

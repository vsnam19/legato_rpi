/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SECURITY_CCS_SERVER_IMPL_HPP
#define SECURITY_CCS_SERVER_IMPL_HPP

#include <mutex>

#include <grpcpp/grpcpp.h>
#include "protos/proto-src/security_simulation.grpc.pb.h"
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "libs/common/JsonParser.hpp"

#include "event/EventService.hpp"
#include "event/ServerEventManager.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

#include "connsec.h"

class SecurityCCSServerImpl : public ::securityStub::SecurityCCSService::Service,
                              public IServerEventListener,
                              public std::enable_shared_from_this<SecurityCCSServerImpl> {

 public:
    SecurityCCSServerImpl();
    ~SecurityCCSServerImpl();

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

    grpc::Status Init(::grpc::ServerContext* context,
        const ::securityStub::CCSConnectInfo* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status DeInit(::grpc::ServerContext* context,
        const ::securityStub::CCSDisconnectInfo* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status GetCurrentSessionStats(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::securityStub::SessionStats* response) override;

    grpc::Status registerCCSListener(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::commonStub::ErrorCodeMsg* response);

    grpc::Status deRegisterCCSListener(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::commonStub::ErrorCodeMsg* response);

    void onEventUpdate(std::string event);

 private:
    const uint32_t SSGCCS_HOSTILE_SCORE = 385;
    const char * const CCS_FILTER = "ccs";
    const char * const CCS_DEFAULT_DELIMITER = " ";
    const char * const CCS_API_JSON_FILE = "api/sec/ICellularSecurityManager.json";

    uint32_t clientsCount_ = 0;
    bool isServiceInitialized_{false};

    ssgccs_session_state_t curSessionStats_{};
    Json::Value apiConfigJsonRoot_;
    std::mutex operationGuard_;
    ServerEventManager &serverEvent_;
    EventService &clientEvent_;

    void handleSSREvent(std::string eventParams);
    void handleSecurityReportEvent(std::string eventParams);
    void toSSGPolicy(uint32_t teluxPolicy, ssgccs_client_policy_action_t &ssgPolicy);
    void toSSGCategory(uint32_t threatTypesBitMask, uint32_t &category);
    std::string toPolicyStr(ssgccs_client_policy_action_t ssgPolicy);
};

#endif // SECURITY_CCS_SERVER_IMPL_HPP

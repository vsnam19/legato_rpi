/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SECURITY_WCS_SERVER_IMPL_HPP
#define SECURITY_WCS_SERVER_IMPL_HPP

#include <mutex>

#include <grpcpp/grpcpp.h>
#include "protos/proto-src/security_simulation.grpc.pb.h"
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "libs/common/JsonParser.hpp"

#include "event/EventService.hpp"
#include "event/ServerEventManager.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

class SecurityWCSServerImpl : public ::securityStub::SecurityWCSService::Service,
                              public IServerEventListener,
                              public std::enable_shared_from_this<SecurityWCSServerImpl> {

 public:
    SecurityWCSServerImpl();
    ~SecurityWCSServerImpl();

    grpc::Status Init(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::securityStub::InitInfo* response) override;

    grpc::Status DeInit(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status RegisterClientForReport(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status DeregisterClientForReport(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status GetTrustedApList(::grpc::ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::securityStub::TrustedAPList* response) override;

    grpc::Status SetTrustedAp(::grpc::ServerContext* context,
        const ::securityStub::IsTrustedUserResponse* request,
        ::commonStub::ErrorCodeMsg* response) override;

    grpc::Status RemoveApFromTrustedList(::grpc::ServerContext* context,
        const ::securityStub::ApInfo* request,
        ::commonStub::ErrorCodeMsg* response) override;

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

 private:
    const char * const WCS_FILTER = "wcs";
    const char * const WCS_DEFAULT_DELIMITER = " ";
    const char * const WCS_MANAGER = "IWiFiSecurityManager";
    const char * const ACCESS_POINTS = "accessPoints";
    const char * const WCS_API_JSON_FILE = "api/sec/IWiFiSecurityManager.json";
    const char * const WCS_DATABASE_FILE = "system-state/sec/IWiFiSecurityManager.json";

    uint32_t clientsCount_ = 0;
    bool isServiceInitialized_{false};

    Json::Value apiConfigJsonRoot_;
    std::mutex operationGuard_;
    ServerEventManager &serverEvent_;
    EventService &clientEvent_;

    void handleSSREvent(std::string eventParams);
    void handleSecurityReportEvent(std::string eventParams);
    void handleDeauthEvent(std::string eventParams);
    void handleIsTrustedAP(std::string eventParams);
};

#endif // SECURITY_WCS_SERVER_IMPL_HPP

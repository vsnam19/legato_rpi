/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef NTN_SERVER_HPP
#define NTN_SERVER_HPP

#include <telux/satcom/NtnManager.hpp>

#include "satcom/NtnServerImpl.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "event/ServerEventManager.hpp"
#include "protos/proto-src/satcom_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class NtnServerImpl final:
    public satcomStub::NtnManager::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<NtnServerImpl> {
public:
    NtnServerImpl();
    ~NtnServerImpl();

    grpc::Status InitService(ServerContext* context,
        const ::google::protobuf::Empty* request,
        satcomStub::GetServiceStatusReply* response) override;

    grpc::Status IsNtnSupported(ServerContext* context,
        const satcomStub::IsNtnSupportedRequest* request,
        satcomStub::IsNtnSupportedReply* response) override;

    grpc::Status EnableNtn(ServerContext* context,
        const ::satcomStub::EnableNtnRequest* request,
        satcomStub::DefaultReply* response) override;

    grpc::Status SendData(ServerContext* context,
        const google::protobuf::Empty* request,
        satcomStub::SendDataReply* response) override;

    grpc::Status AbortData(ServerContext* context,
        const google::protobuf::Empty* request,
        satcomStub::DefaultReply* response) override;

    grpc::Status GetNtnCapabilities(ServerContext* context,
        const google::protobuf::Empty* request,
        satcomStub::GetNtnCapabilitiesReply* response) override;

    grpc::Status GetSignalStrength(ServerContext* context,
        const google::protobuf::Empty* request,
        satcomStub::GetSignalStrengthReply* response) override;

    grpc::Status UpdateSystemSelectionSpecifiers(ServerContext* context,
        const ::google::protobuf::Empty* request,
        satcomStub::DefaultReply* response) override;

    grpc::Status GetNtnState(ServerContext* context,
        const google::protobuf::Empty* request,
        satcomStub::GetNtnStateReply* response) override;

    grpc::Status EnableCellularScan(ServerContext* context,
        const satcomStub::EnableCellularScanRequest* request,
        satcomStub::DefaultReply* response) override;

    grpc::Status SetLocationFix(ServerContext* context,
        const satcomStub::SetLocationFixRequest* request,
        satcomStub::DefaultReply* response) override;

    grpc::Status LocationFixResponse(ServerContext* context,
        const satcomStub::LocationFixResponseRequest* request,
        satcomStub::DefaultReply* response) override;

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    bool enableCellularScan_ = false;
    telux::satcom::NtnState ntnState_ = telux::satcom::NtnState::DISABLED;
    int capabilities_ = 0;
    telux::satcom::SignalStrength signalStrength_ = telux::satcom::SignalStrength::NONE;

    void onEventUpdate(std::string event);
    void handleStateChangeRequest(std::string event);
    void handleCellularCoverageAvailable(std::string event);
    void handleLocationFixRequest(std::string event);
    void handleIncomingData(std::string event);

    uint64_t generateRandomTransactionId();
};

#endif //NTN_SERVER_HPP

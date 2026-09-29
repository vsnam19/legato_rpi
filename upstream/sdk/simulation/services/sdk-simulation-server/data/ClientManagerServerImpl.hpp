/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CLIENT_MANAGER_SERVER_IMPL_HPP
#define CLIENT_MANAGER_SERVER_IMPL_HPP

#include <mutex>
#include <memory>
#include <vector>

#include <telux/data/ClientManager.hpp>
#include "protos/proto-src/data_simulation.grpc.pb.h"
#include "libs/common/AsyncTaskQueue.hpp"
#include "event/ServerEventManager.hpp"


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;



class ClientManagerServerImpl final :
    public dataStub::ClientManager::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<ClientManagerServerImpl> {

public:
    ClientManagerServerImpl();
    ~ClientManagerServerImpl();

    grpc::Status InitService(ServerContext* context,
        const ::dataStub::InitRequest* request,
        ::dataStub::GetServiceStatusReply* response) override;

    grpc::Status GetDeviceDataUsageStats(ServerContext* context,
        const ::google::protobuf::Empty* request,
        dataStub::GetDeviceDataUsageStatsResponse* response) override;

    grpc::Status ResetDataUsageStats(ServerContext* context,
        const dataStub::ResetDataUsageStatsRequest* request,
        dataStub::ResetDataUsageStatsResponse* response) override;

    void onEventUpdate(::eventService::UnsolicitedEvent message) override;



private:
    std::mutex mutex_;
    void onEventUpdate(std::string event);
    void handleDeviceDataUsageStatsUpdate(std::string event);
    void handleDeviceDataUsageReset(std::string event);
    std::vector<std::string> splitBySpace(const std::string &input);
    bool isMonitoringEnabled();
    std::shared_ptr<ClientManagerServerImpl> clientManagerImpl_;
    telux::common::ServiceStatus subSystemStatus_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;


};

#endif // CLIENT_MANAGER_SERVER_IMPL_HPP
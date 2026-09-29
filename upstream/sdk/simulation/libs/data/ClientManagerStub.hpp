/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CLIENT_MANGER_STUB_HPP
#define CLIENT_MANGER_STUB_HPP

#include <telux/data/ClientManager.hpp>
#include <telux/data/DataDefines.hpp>
#include <thread>
#include <chrono>
#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

using ::dataStub::ClientManager;

namespace telux {
namespace data {

class ClientManagerStub : public IClientManager,
                          public IClientListener,
                          public telux::common::IEventListener,
                          public std::enable_shared_from_this<ClientManagerStub> {
 public:
    ClientManagerStub();
    ~ClientManagerStub();

    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(std::weak_ptr<IClientListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IClientListener> listener) override;
    telux::common::ErrorCode getDeviceDataUsageStats(
        std::vector<DeviceDataUsage> &usageStats) override;
    telux::common::ErrorCode resetDataUsageStats() override;
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    telux::common::Status cleanup();
    telux::common::Status init(telux::common::InitResponseCb callback);
    void onEventUpdate(google::protobuf::Any event);

 private:
    void initSync(telux::common::InitResponseCb callback);
    void setSubsystemReady(bool status);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void invokeInitCallback(telux::common::ServiceStatus status);
    void handleDeviceDataUsageReset(
        ::dataStub::DeviceDataUsageResetEvent &deviceDataUsageResetEvent);

    std::mutex mtx_;
    bool isInitComplete_;
    SlotId slotId_ = DEFAULT_SLOT_ID;
    telux::data::OperationType oprType_;
    telux::common::ServiceStatus subSystemStatus_;
    telux::common::InitResponseCb initCb_;
    std::unique_ptr<::dataStub::ClientManager::Stub> stub_;
    std::weak_ptr<IClientListener> listeners_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::shared_ptr<telux::common::ListenerManager<IClientListener>> listenerMgr_;

};
}  // namespace data
}  // namespace telux

#endif
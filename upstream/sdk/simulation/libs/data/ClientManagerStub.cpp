/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <ClientManagerStub.hpp>
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"
#include "DataUtilsStub.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1
#define CLIENT_MANAGER "client_manager"
#define CLIENT_MANAGER_STATE_JSON "system-state/data/IClientManagerState.json"

namespace telux {
namespace data {

ClientManagerStub::ClientManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IClientListener>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

ClientManagerStub::~ClientManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status ClientManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this, callback]() {
        this->initSync(callback);}).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void ClientManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(mtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::ClientManager>();

    ::dataStub::InitRequest request;
    ::dataStub::GetServiceStatusReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    int cbDelay = DEFAULT_DELAY;

    do {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " InitService request failed");
            break;
        }

        cbStatus =
            static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay = static_cast<int>(response.delay());

        this->onServiceStatusChange(cbStatus);
        LOG(DEBUG, __FUNCTION__, " ServiceStatus: ", static_cast<int>(cbStatus));
    } while (0);

    setSubSystemStatus(cbStatus);

      std::vector<std::string> filters = {CLIENT_MANAGER};
    auto &clientEventManager = telux::common::ClientEventManager::getInstance();
    clientEventManager.registerListener(shared_from_this(), filters);

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay,
            " cbStatus::", static_cast<int>(cbStatus));
        invokeInitCallback(cbStatus);
    }
}


telux::common::ServiceStatus ClientManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(subSystemStatus_));
    return subSystemStatus_;
}


void ClientManagerStub::onServiceStatusChange(
    telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IClientListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Client Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::Status ClientManagerStub::registerListener(
    std::weak_ptr<IClientListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status ClientManagerStub::deregisterListener(
    std::weak_ptr<IClientListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

void ClientManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}


telux::common::ErrorCode ClientManagerStub::getDeviceDataUsageStats(
    std::vector<DeviceDataUsage> &usageStats) {

    LOG(INFO, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR,__FUNCTION__, " Client manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::google::protobuf::Empty request;
    dataStub::GetDeviceDataUsageStatsResponse response;
    ClientContext context;

    grpc::Status status = stub_->GetDeviceDataUsageStats(&context, request, &response);

    if (!status.ok()) {
        LOG(ERROR, __FUNCTION__, "getDeviceDataUsageStats failed: ", status.error_message());
        return telux::common::ErrorCode::INVALID_STATE;
    }

    LOG(DEBUG,__FUNCTION__,"response.usage_stats_size(): ",response.usage_stats_size());
    for (int i = 0; i < response.usage_stats_size(); ++i) {
        const auto &entry = response.usage_stats(i);
        DeviceDataUsage usage;
        usage.macAddress = entry.mac_address();
        usage.usage.bytesRx = entry.usage().bytes_rx();
        usage.usage.bytesTx = entry.usage().bytes_tx();
        usageStats.push_back(usage);
    }
    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode ClientManagerStub::resetDataUsageStats() {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR,__FUNCTION__, " Client manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::dataStub::ResetDataUsageStatsRequest request;
    ::dataStub::ResetDataUsageStatsResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub_->ResetDataUsageStats(&context, request, &response);

    if (!status.ok()) {
        LOG(ERROR, __FUNCTION__, " gRPC failed: ", status.error_message());
        return telux::common::ErrorCode::INTERNAL_ERROR;
    }

    return static_cast<telux::common::ErrorCode>(response.error());
}

void ClientManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    subSystemStatus_ = status;
}
void ClientManagerStub::onEventUpdate(google::protobuf::Any event) {

    if (event.Is<::dataStub::DeviceDataUsageResetEvent>()) {
        ::dataStub::DeviceDataUsageResetEvent deviceDataUsageResetEvent;
        event.UnpackTo(&deviceDataUsageResetEvent);
        this->handleDeviceDataUsageReset(deviceDataUsageResetEvent);
    }

}

void ClientManagerStub::handleDeviceDataUsageReset(
        ::dataStub::DeviceDataUsageResetEvent &event) {

    telux::data::DeviceDataUsage usage;
    usage.macAddress = event.mac_address();
    usage.usage.bytesRx = event.bytes_rx();
    usage.usage.bytesTx = event.bytes_tx();

     std::string reason = event.reason();

    UsageResetReason resetReason;
    if (reason == "SUBSYSTEM_UNAVAILABLE") {
        resetReason = UsageResetReason::SUBSYSTEM_UNAVAILABLE;
    } else if (reason == "BACKHAUL_SWITCHED") {
        resetReason = UsageResetReason::BACKHAUL_SWITCHED;
    } else if (reason == "DEVICE_DISCONNECTED") {
        resetReason = UsageResetReason::DEVICE_DISCONNECTED;
    } else if (reason == "WLAN_DISABLED") {
        resetReason = UsageResetReason::WLAN_DISABLED;
    } else if (reason == "WWAN_DISCONNECTED") {
        resetReason = UsageResetReason::WWAN_DISCONNECTED;
    }

    if (listenerMgr_) {
        std::vector<std::weak_ptr<IClientListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, "Notifying ", listeners.size(), " listeners");

        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                sp->onDeviceDataUsageResetImminent({usage}, resetReason);
            }
        }
    }
}



} // end of namespace data
} // end of namespace telux

/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <regex>
#include <fstream>

#include "telux/power/PowerFactory.hpp"

#include "common/Logger.hpp"
#include "TcuActivityManagerImpl.hpp"

namespace telux {
namespace power {

using namespace telux::common;

TcuActivityManagerImpl::TcuActivityManagerImpl(ClientInstanceConfig config)
   : grpcClient_(nullptr)
   , config_(config) {
    if (config_.clientType == ClientType::MASTER) {
        config_.machineName = LOCAL_MACHINE;
    }
    LOG(INFO, __FUNCTION__, " Client name: ", config_.clientName,
        ", Client type: ", static_cast<int>(config_.clientType),
        ", machine name: ", config_.machineName);
}

TcuActivityManagerImpl::~TcuActivityManagerImpl() {
    LOG(INFO, __FUNCTION__);
}

bool TcuActivityManagerImpl::isReady() {
    LOG(WARNING, __FUNCTION__, " deprecated API used!");
    std::lock_guard<std::mutex> lock(mutex_);
    return (subSystemStatus_ == telux::common::ServiceStatus::SERVICE_AVAILABLE);
}

telux::common::ServiceStatus TcuActivityManagerImpl::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mutex_);
    return subSystemStatus_;
}

bool TcuActivityManagerImpl::waitForInitialization() {
    LOG(DEBUG, __FUNCTION__);
    std::unique_lock<std::mutex> cvLock(mutex_);
    while (subSystemStatus_ != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        initCV_.wait(cvLock);
    }
    return true;
}

std::future<bool> TcuActivityManagerImpl::onReady() {
    LOG(WARNING, __FUNCTION__, " deprecated API used!");
    auto f = std::async(std::launch::async, [&] { return waitForInitialization(); });
    return f;
}

void TcuActivityManagerImpl::setServiceStatusAndNotify(telux::common::ServiceStatus status) {
    bool notifyListeners = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (subSystemStatus_ != status) {
            notifyListeners = true;
        }
        subSystemStatus_ = status;
        if (status != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            isInitsyncTriggered_ = false;
        }
    }
    if (initCb_) {
        initCb_(status);
    } else {
        LOG(ERROR, __FUNCTION__, " Callback is NULL");
    }
    if (notifyListeners) {
        std::vector<std::weak_ptr<IServiceStatusListener>> applisteners;
        if (svcStatusListenerMgr_) {
            svcStatusListenerMgr_->getAvailableListeners(applisteners);
            for (auto &wp : applisteners) {
                if (auto sp = std::dynamic_pointer_cast<IServiceStatusListener>(wp.lock())) {
                    sp->onServiceStatusChange(status);
                }
            }
        } else {
            LOG(ERROR, __FUNCTION__, " svcStatusListenerMgr is null");
        }
    }
}

telux::common::Status TcuActivityManagerImpl::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;

    std::lock_guard<std::mutex> lock(mutex_);
    initCb_      = callback;
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<ITcuActivityListener>>();
    if (!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " FAILED to create ListenerManager instance");
        cleanup();
        initCb_(telux::common::ServiceStatus::SERVICE_FAILED);
        return telux::common::Status::FAILED;
    }

    svcStatusListenerMgr_ = std::make_shared<telux::common::ListenerManager<IServiceStatusListener>>();
    if (!svcStatusListenerMgr_) {
        LOG(ERROR, __FUNCTION__, " FAILED to create service state ListenerManager instance");
        cleanup();
        initCb_(telux::common::ServiceStatus::SERVICE_FAILED);
        return telux::common::Status::FAILED;
    }

    grpcClient_ = std::make_shared<PowerGrpcClient>(
        static_cast<int>(config_.clientType), config_.clientName, config_.machineName);

    if (!grpcClient_) {
        LOG(ERROR, __FUNCTION__, " FAILED to get TCU-activity GRPC client");
        cleanup();
        initCb_(telux::common::ServiceStatus::SERVICE_FAILED);
        return telux::common::Status::FAILED;
    }
    status = grpcClient_->registerListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " FAILED to register a TCU-activity GRPC Listener");
        cleanup();
        initCb_(telux::common::ServiceStatus::SERVICE_FAILED);
        return status;
    }
    auto f = std::async(std::launch::async, [this]() { this->initSync(); }).share();
    status = taskQ_.add(f);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Failed to add initSync task to AsyncTaskQueue");
        cleanup();
        initCb_(telux::common::ServiceStatus::SERVICE_FAILED);
        return status;
    }
    return status;
}

void TcuActivityManagerImpl::initSync() {
    LOG(DEBUG, __FUNCTION__);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (isInitsyncTriggered_) {
            LOG(DEBUG, __FUNCTION__, " Initialization is already triggered");
            return;
        } else {
            isInitsyncTriggered_ = true;
        }
    }
    if (grpcClient_ == nullptr) {
        LOG(ERROR, __FUNCTION__, " Power GRPC client is null");
        cleanup(false);
        setServiceStatusAndNotify(telux::common::ServiceStatus::SERVICE_FAILED);
        return;
    }
    bool isSvcReady = grpcClient_->isReady();
    if (!isSvcReady) {
        std::future<bool> f = grpcClient_->onReady();
        isSvcReady          = f.get();
    }
    if (!isSvcReady) {
        LOG(ERROR, __FUNCTION__, " Failed to initialize PowerGrpcClient");
        cleanup(false);
        setServiceStatusAndNotify(telux::common::ServiceStatus::SERVICE_FAILED);
        return;
    }

    do {
        TcuActivityState initialState;
        auto status = grpcClient_->registerTcuStateEvents(initialState);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " FAILED to register for TCU-activity state events");
        }
        setServiceStatusAndNotify(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        initCV_.notify_all();
        return;
    } while (false);
    cleanup(false);
    setServiceStatusAndNotify(telux::common::ServiceStatus::SERVICE_FAILED);
}

void TcuActivityManagerImpl::cleanup(bool isExiting) {
    LOG(INFO, __FUNCTION__);
    if (grpcClient_) {
        grpcClient_->deregisterListener(shared_from_this());
        grpcClient_ = nullptr;
    }
    if (isExiting) {
        taskQ_.shutdown();
        svcStatusListenerMgr_ = nullptr;
    }
    listenerMgr_ = nullptr;
}

Status TcuActivityManagerImpl::registerListener(std::weak_ptr<ITcuActivityListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
    }
    return status;
}

Status TcuActivityManagerImpl::deregisterListener(std::weak_ptr<ITcuActivityListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->deRegisterListener(listener);
    }
    return status;
}

Status TcuActivityManagerImpl::registerServiceStateListener(
    std::weak_ptr<IServiceStatusListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (svcStatusListenerMgr_) {
        status = svcStatusListenerMgr_->registerListener(listener);
    }
    return status;
}

Status TcuActivityManagerImpl::deregisterServiceStateListener(
    std::weak_ptr<IServiceStatusListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (svcStatusListenerMgr_) {
        status = svcStatusListenerMgr_->deRegisterListener(listener);
    }
    return status;
}

telux::common::Status TcuActivityManagerImpl::setActivityState(
    TcuActivityState state, std::string machineName, telux::common::ResponseCallback callback) {
    LOG(INFO, __FUNCTION__, " machine name: ", machineName, " state: ", static_cast<int>(state));
    telux::common::Status status = telux::common::Status::FAILED;
    status = grpcClient_->sendActivityStateCommand(state, machineName, callback);
    return status;
}

telux::common::Status TcuActivityManagerImpl::setActivityState(
    TcuActivityState state, telux::common::ResponseCallback callback) {
    LOG(WARNING, __FUNCTION__, " deprecated API used!");
    return telux::common::Status::NOTSUPPORTED;
}

TcuActivityState TcuActivityManagerImpl::getActivityState() {
    TcuActivityState state;
    getActivityState("PVM", state);
    return state;
}

telux::common::Status TcuActivityManagerImpl::sendActivityStateAck(
    StateChangeResponse ack, TcuActivityState state) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    status = grpcClient_->sendActivityStateAck(ack, state);
    return status;
}

telux::common::Status TcuActivityManagerImpl::sendActivityStateAck(TcuActivityStateAck ack) {
    LOG(WARNING, __FUNCTION__, " deprecated API used!");
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status TcuActivityManagerImpl::setModemActivityState(TcuActivityState state) {
    LOG(INFO, __FUNCTION__, " state: ", static_cast<int>(state));
    telux::common::Status status = telux::common::Status::FAILED;
    status = grpcClient_->setModemActivityState(state);
    return status;
}

telux::common::Status TcuActivityManagerImpl::getMachineName(std::string &machineName) {
    LOG(DEBUG, __FUNCTION__);
    machineName = "PVM";
    return telux::common::Status::SUCCESS;
}

telux::common::Status TcuActivityManagerImpl::getAllMachineNames(
    std::vector<std::string> &machineNames) {
    LOG(DEBUG, __FUNCTION__);
    machineNames.push_back("PVM");
    return telux::common::Status::SUCCESS;
}

telux::common::ErrorCode TcuActivityManagerImpl::getActivityState(std::string machineName,
    TcuActivityState &state) {
    LOG(DEBUG, __FUNCTION__);
    if((config_.clientType == ClientType::SLAVE) && (machineName != "PVM")) {
        return telux::common::ErrorCode::OPERATION_NOT_ALLOWED;
    }
    if(machineName != "PVM") {
        LOG(DEBUG, __FUNCTION__, " machinename not found ");
        return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }
    telux::common::Status status            = telux::common::Status::FAILED;
    status = grpcClient_->getActivityState(state);
    if(status == telux::common::Status::SUCCESS) {
        return telux::common::ErrorCode::SUCCESS;
    }
    LOG(DEBUG, __FUNCTION__, " Failed - ", static_cast<int>(status));
    return telux::common::ErrorCode::GENERIC_FAILURE;
}

void TcuActivityManagerImpl::onMachineUpdate(telux::power::MachineEvent state) {
    LOG(DEBUG, __FUNCTION__);
    std::string tcuMachineName = LOCAL_MACHINE;
    std::vector<std::weak_ptr<ITcuActivityListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }

    for (auto &wp : applisteners) {
        if (auto sp = wp.lock()) {
            sp->onMachineUpdate(tcuMachineName, state);
        }
    }
}

void TcuActivityManagerImpl::onTcuStateUpdate(TcuActivityState state, std::string tcuMachineName) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ITcuActivityListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
    // If there are no listeners, send acknowledgement
    if (applisteners.size() == 0) {
        LOG(DEBUG, __FUNCTION__ , " Sending ACK");
        if (state == TcuActivityState::RESUME) {
            return;
        }
        auto status = sendActivityStateAck(StateChangeResponse::ACK, state);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Failed to send TCU-activity state acknowledgement");
        }
    } else {
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onTcuActivityStateUpdate(state, tcuMachineName);
            }
        }
    }
}

void TcuActivityManagerImpl::onSlaveAckStatusUpdate(std::vector<std::string> nackList,
    std::vector<std::string> noackList, std::string machineName) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::SUCCESS;
    std::vector<ClientInfo> nackResponseClients;
    std::vector<ClientInfo> unresponsiveClients;

    for(auto client: nackList) {
        nackResponseClients.push_back({client, machineName});
    }

    for(auto client: noackList) {
        unresponsiveClients.push_back({client, machineName});
    }

    std::vector<std::weak_ptr<ITcuActivityListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }

    if(!nackResponseClients.empty() || !unresponsiveClients.empty()) {
        status = telux::common::Status::NOTREADY;
    }

    for (auto &wp : applisteners) {
        if (auto sp = wp.lock()) {
            sp->onSlaveAckStatusUpdate(
                status, machineName, unresponsiveClients, nackResponseClients);
        }
    }
}

}  // end of namespace power
}  // end of namespace telux

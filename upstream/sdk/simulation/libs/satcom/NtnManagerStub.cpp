/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "NtnManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"
#include <thread>
#include <chrono>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1
#define NTN_FILTER "ntn"

namespace telux {
namespace satcom {

NtnManagerStub::NtnManagerStub()
{
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<INtnListener>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

NtnManagerStub::~NtnManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status NtnManagerStub::init(telux::common::InitResponseCb callback)
{
    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this, callback]() {
        this->initSync(callback);}).share();
    taskQ_->add(f);
    return telux::common::Status::SUCCESS;
}

void NtnManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::satcomStub::NtnManager>();
    ::satcomStub::GetServiceStatusReply response;
    ClientContext context;
    ::google::protobuf::Empty request{};
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

    if(cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {NTN_FILTER};
        auto &clientEventManager = telux::common::ClientEventManager::getInstance();
        clientEventManager.registerListener(shared_from_this(), filters);
    }

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay,
            " cbStatus::", static_cast<int>(cbStatus));
        invokeInitCallback(cbStatus);
    }
}

void NtnManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}
void NtnManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus NtnManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

telux::common::ErrorCode NtnManagerStub::isNtnSupported(bool &isSupported) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ntn manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::satcomStub::IsNtnSupportedRequest request;
    ::satcomStub::IsNtnSupportedReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->IsNtnSupported(&context, request, &response);

    telux::common::ErrorCode error =
        static_cast<telux::common::ErrorCode>(response.reply().error());

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " isNtnSupported request failed");
        error = telux::common::ErrorCode::INTERNAL_ERROR;
    }

    if (error == telux::common::ErrorCode::SUCCESS) {
        isSupported = response.is_supported();
    }

    return error;
}

telux::common::ErrorCode NtnManagerStub::enableNtn(
    bool enable, bool isEmergency, const std::string &iccid) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ntn manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::satcomStub::EnableNtnRequest request;
    ::satcomStub::DefaultReply response;
    ClientContext context;

    request.set_enable(enable);
    grpc::Status reqStatus = stub_->EnableNtn(&context, request, &response);

    telux::common::ErrorCode error =
        static_cast<telux::common::ErrorCode>(response.error());

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " enableNtn request failed");
        error = telux::common::ErrorCode::INTERNAL_ERROR;
    }

    return error;
}

telux::common::Status NtnManagerStub::sendData(
    uint8_t *data, uint32_t size, bool isEmergency, TransactionId &TransactionId)
{
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ntn manager not ready");
        return telux::common::Status::NOTREADY;
    }

    ::google::protobuf::Empty request;
    ::satcomStub::SendDataReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->SendData(&context, request, &response);

    telux::common::Status status =
        static_cast<telux::common::Status>(response.reply().status());
    telux::common::ErrorCode error =
        static_cast<telux::common::ErrorCode>(response.reply().error());

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " SendData request failed");
        status = telux::common::Status::FAILED;
        return status;
    }

    if (error == telux::common::ErrorCode::SUCCESS) {
        TransactionId = response.transaction_id();
        auto task = std::async(std::launch::async, [this, error, TransactionId] {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            onDataAck(error, TransactionId);
        }).share();
        taskQ_->add(task);
    }

    return status;
}

telux::common::ErrorCode NtnManagerStub::abortData() {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ntn manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::google::protobuf::Empty request;
    ::satcomStub::DefaultReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->AbortData(&context, request, &response);

    telux::common::ErrorCode error =
        static_cast<telux::common::ErrorCode>(response.error());

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " abortData request failed");
        error = telux::common::ErrorCode::INTERNAL_ERROR;
    }

    return error;
}

telux::common::ErrorCode NtnManagerStub::getNtnCapabilities(NtnCapabilities &capabilities) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ntn manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::google::protobuf::Empty request;
    ::satcomStub::GetNtnCapabilitiesReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->GetNtnCapabilities(&context, request, &response);

    telux::common::ErrorCode error =
        static_cast<telux::common::ErrorCode>(response.reply().error());

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " getNtnCapabilities request failed");
        error = telux::common::ErrorCode::INTERNAL_ERROR;
    }

    if (error == telux::common::ErrorCode::SUCCESS) {
        capabilities.maxDataSize = response.capabilities().max_data_size();
    }

    return error;
}

telux::common::ErrorCode NtnManagerStub::getSignalStrength(SignalStrength &signalStrength) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ntn manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::google::protobuf::Empty request;
    ::satcomStub::GetSignalStrengthReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->GetSignalStrength(&context, request, &response);

    telux::common::ErrorCode error =
        static_cast<telux::common::ErrorCode>(response.reply().error());

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " getSignalStrength request failed");
        error = telux::common::ErrorCode::INTERNAL_ERROR;
    }

    if (error == telux::common::ErrorCode::SUCCESS) {
        signalStrength = static_cast<SignalStrength>(response.signal_strength());
    }

    return error;
}

telux::common::ErrorCode NtnManagerStub::updateSystemSelectionSpecifiers(
    std::vector<SystemSelectionSpecifier> &params) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ntn manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::google::protobuf::Empty request;
    ::satcomStub::DefaultReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->UpdateSystemSelectionSpecifiers(&context, request, &response);

    telux::common::ErrorCode error =
        static_cast<telux::common::ErrorCode>(response.error());

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " updateSystemSelectionSpecifiers request failed");
        error = telux::common::ErrorCode::INTERNAL_ERROR;
    }

    return error;
}

NtnState NtnManagerStub::getNtnState() {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ntn manager not ready");
        return NtnState::DISABLED;
    }

    NtnState state = NtnState::DISABLED;
    ::google::protobuf::Empty request;
    ::satcomStub::GetNtnStateReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->GetNtnState(&context, request, &response);

    if (reqStatus.ok()) {
        state = static_cast<NtnState>(response.state());
        LOG(DEBUG, __FUNCTION__, " state:", static_cast<int>(state));
    } else {
        LOG(ERROR, __FUNCTION__, " getNtnState request failed");
    }

    return state;
}

telux::common::ErrorCode NtnManagerStub::enableCellularScan(bool enable) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ntn manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::satcomStub::EnableCellularScanRequest request;
    ::satcomStub::DefaultReply response;
    ClientContext context;

    request.set_enable(enable);

    grpc::Status reqStatus = stub_->EnableCellularScan(&context, request, &response);

    telux::common::ErrorCode error =
        static_cast<telux::common::ErrorCode>(response.error());

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " enableCellularScan request failed");
        error = telux::common::ErrorCode::INTERNAL_ERROR;
    }

    return error;
}

telux::common::ErrorCode NtnManagerStub::setLocationFix(const LocationFix &params) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ntn manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::satcomStub::SetLocationFixRequest request;
    ::satcomStub::DefaultReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->SetLocationFix(&context, request, &response);

    telux::common::ErrorCode error =
        static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setLocationFix request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
    }

    return error;
}

telux::common::ErrorCode NtnManagerStub::locationFixResponse(LocationStatus status,
    uint64_t waitTime) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ntn manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::satcomStub::LocationFixResponseRequest request;
    ::satcomStub::DefaultReply response;
    ClientContext context;

    request.set_status(static_cast<::satcomStub::LocationStatus>(status));
    request.set_wait_time(waitTime);

    grpc::Status reqStatus = stub_->LocationFixResponse(&context, request, &response);

    telux::common::ErrorCode error =
        static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " locationFixResponse request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
    }

    return error;
}

void NtnManagerStub::onIncomingData(std::unique_ptr<uint8_t[]> data, uint32_t size) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<INtnListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Ntn Manager: invoking onIncomingData");
                sp->onIncomingData(std::move(data), size);
            }
        }
    }
}

void NtnManagerStub::onDataAck(telux::common::ErrorCode err, telux::satcom::TransactionId id) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<INtnListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Ntn Manager: invoking onDataAck");
                sp->onDataAck(err, id);
            }
        }
    }
}

void NtnManagerStub::onSignalStrengthChange(telux::satcom::SignalStrength newStrength) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<INtnListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Ntn Manager: invoking onSignalStrengthChange");
                sp->onSignalStrengthChange(newStrength);
            }
        }
    }
}

void NtnManagerStub::onCapabilitiesChange(telux::satcom::NtnCapabilities capabilities) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<INtnListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Ntn Manager: invoking onCapabilitiesChange");
                sp->onCapabilitiesChange(capabilities);
            }
        }
    }
}

void NtnManagerStub::onNtnStateChange(telux::satcom::NtnState state) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<INtnListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Ntn Manager: invoking onNtnStateChange");
                sp->onNtnStateChange(state);
            }
        }
    }
}

void NtnManagerStub::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<INtnListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Ntn Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

void NtnManagerStub::onCellularCoverageAvailable(bool isCellularCoverageAvailable) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<INtnListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Ntn Manager: invoking onCellularCoverageAvailable");
                sp->onCellularCoverageAvailable(isCellularCoverageAvailable);
            }
        }
    }
}

void NtnManagerStub::onLocationFixRequest(LocationFixRequestReason reqReason) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<INtnListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Ntn Manager: invoking onLocationFixRequest");
                sp->onLocationFixRequest(reqReason);
            }
        }
    }
}

void NtnManagerStub::onNtnBandUpdate(uint32_t bandValue) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<INtnListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Ntn Manager: invoking onNtnBandUpdate");
                sp->onNtnBandUpdate(bandValue);
            }
        }
    }
}

// Registration APIs
telux::common::Status NtnManagerStub::registerListener(std::weak_ptr<INtnListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status NtnManagerStub::deregisterListener(std::weak_ptr<INtnListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

void NtnManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::satcomStub::NtnStateEvent>()) {
        ::satcomStub::NtnStateEvent ntnStateEvent;
        event.UnpackTo(&ntnStateEvent);
        this->handleNtnStateChangeEvent(ntnStateEvent);
    } else if (event.Is<::satcomStub::CellularCoverageAvailableEvent>()) {
        ::satcomStub::CellularCoverageAvailableEvent cellularCoverageAvailableEvent;
        event.UnpackTo(&cellularCoverageAvailableEvent);
        this->handleCellularCoverageAvailableEvent(cellularCoverageAvailableEvent);
    }  else if (event.Is<::satcomStub::LocationFixRequestEvent>()) {
        ::satcomStub::LocationFixRequestEvent locationFixRequestEvent;
        event.UnpackTo(&locationFixRequestEvent);
        this->handleLocationFixRequestEvent(locationFixRequestEvent);
    } else if (event.Is<::satcomStub::IncomingDataEvent>()) {
        ::satcomStub::IncomingDataEvent incomingDataEvent;
        event.UnpackTo(&incomingDataEvent);
        this->handleIncomingDataEvent(incomingDataEvent);
    }
}

void NtnManagerStub::handleNtnStateChangeEvent(::satcomStub::NtnStateEvent ntnStateEvent) {
    LOG(DEBUG, __FUNCTION__);
    NtnState state = static_cast<NtnState>(ntnStateEvent.state());
    NtnCapabilities capabilities;
    capabilities.maxDataSize = ntnStateEvent.capabilities();
    SignalStrength signalStrength = static_cast<SignalStrength>(ntnStateEvent.signal_strength());
    uint32_t bandValue = ntnStateEvent.band_value();

    onNtnStateChange(state);
    onCapabilitiesChange(capabilities);
    onSignalStrengthChange(signalStrength);
    onNtnBandUpdate(bandValue);
}

void NtnManagerStub::handleCellularCoverageAvailableEvent(
    ::satcomStub::CellularCoverageAvailableEvent cellularCoverageAvailableEvent) {
    LOG(DEBUG, __FUNCTION__);
    bool isAvailable = cellularCoverageAvailableEvent.is_available();
    onCellularCoverageAvailable(isAvailable);
}

void NtnManagerStub::handleLocationFixRequestEvent(
    ::satcomStub::LocationFixRequestEvent locationFixRequestEvent) {
    LOG(DEBUG, __FUNCTION__);
    LocationFixRequestReason reqReason =
        static_cast<LocationFixRequestReason>(locationFixRequestEvent.req_reason());
    onLocationFixRequest(reqReason);
}

void NtnManagerStub::handleIncomingDataEvent(
    ::satcomStub::IncomingDataEvent incomingDataEvent) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<uint8_t> data;
    for (int i = 0; i < incomingDataEvent.data_size(); i++) {
        data.push_back(static_cast<uint8_t>(incomingDataEvent.data(i)));
    }
    uint8_t* dataArray = new uint8_t[data.size()];
    std::copy(data.begin(), data.end(), dataArray);
    onIncomingData(std::unique_ptr<uint8_t[]>(dataArray), data.size());
}

}  // namespace satcom
}  // namespace telux

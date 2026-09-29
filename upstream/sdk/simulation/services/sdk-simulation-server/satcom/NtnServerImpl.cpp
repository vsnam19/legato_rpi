/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <chrono>

#include "NtnServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "event/EventService.hpp"

#define SATCOM_API_LOCAL_JSON "api/satcom/INtnManager.json"
#define SATCOM_STATE_JSON "system-state/satcom/INtnManagerState.json"
#define DEFAULT_DELIMITER " "
#define NTN_FILTER "ntn"
#define NTN_IN_SERVICE "2"
#define NTN_DISABLED "0"

NtnServerImpl::NtnServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

NtnServerImpl::~NtnServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status NtnServerImpl::InitService(ServerContext* context,
    const ::google::protobuf::Empty* request, satcomStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = SATCOM_API_LOCAL_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["INtnManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["INtnManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<satcomStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {NTN_FILTER};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }

    return grpc::Status::OK;
}

grpc::Status NtnServerImpl::IsNtnSupported(ServerContext* context,
    const satcomStub::IsNtnSupportedRequest* request,
    satcomStub::IsNtnSupportedReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = SATCOM_API_LOCAL_JSON;
    std::string stateJsonPath = SATCOM_STATE_JSON;
    std::string subsystem = "INtnManager";
    std::string method = "isNtnSupported";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        response->set_is_supported(
            data.stateRootObj[subsystem][method]["isSupported"].asBool());
    }

    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status NtnServerImpl::EnableNtn(ServerContext* context,
    const ::satcomStub::EnableNtnRequest* request, satcomStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = SATCOM_API_LOCAL_JSON;
    std::string stateJsonPath = SATCOM_STATE_JSON;
    std::string subsystem = "INtnManager";
    std::string method = "enableNtn";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    bool enable = request->enable();

    std::thread([this, enable] {
        if (enable) {
            handleStateChangeRequest(NTN_IN_SERVICE);
        } else {
            handleStateChangeRequest(NTN_DISABLED);
        }
    }).detach();

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status NtnServerImpl::SendData(ServerContext* context,
    const ::google::protobuf::Empty* request, satcomStub::SendDataReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = SATCOM_API_LOCAL_JSON;
    std::string stateJsonPath = SATCOM_STATE_JSON;
    std::string subsystem = "INtnManager";
    std::string method = "sendData";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (ntnState_ != telux::satcom::NtnState::IN_SERVICE) {
        return grpc::Status(grpc::StatusCode::INTERNAL, " ntn not enabled");
    }

    uint64_t transactionId = generateRandomTransactionId();
    response->set_transaction_id(transactionId);
    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

uint64_t NtnServerImpl::generateRandomTransactionId() {
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(nullptr)));
        seeded = true;
    }

    return (rand() % 1001) + 1000;
}

grpc::Status NtnServerImpl::AbortData(ServerContext* context,
    const google::protobuf::Empty* request, satcomStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = SATCOM_API_LOCAL_JSON;
    std::string stateJsonPath = SATCOM_STATE_JSON;
    std::string subsystem = "INtnManager";
    std::string method = "abortData";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (ntnState_ != telux::satcom::NtnState::IN_SERVICE) {
        response->set_error(
            static_cast<commonStub::ErrorCode>(telux::common::ErrorCode::GENERIC_FAILURE));
        return grpc::Status::OK;
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status NtnServerImpl::GetNtnCapabilities(ServerContext* context,
    const google::protobuf::Empty* request, satcomStub::GetNtnCapabilitiesReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = SATCOM_API_LOCAL_JSON;
    std::string stateJsonPath = SATCOM_STATE_JSON;
    std::string subsystem = "INtnManager";
    std::string method = "getNtnCapabilities";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        response->mutable_capabilities()->set_max_data_size(capabilities_);
    }

    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status NtnServerImpl::GetSignalStrength(ServerContext* context,
    const google::protobuf::Empty* request, satcomStub::GetSignalStrengthReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = SATCOM_API_LOCAL_JSON;
    std::string stateJsonPath = SATCOM_STATE_JSON;
    std::string subsystem = "INtnManager";
    std::string method = "getSignalStrength";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        response->set_signal_strength(
            static_cast<satcomStub::SignalStrength>(signalStrength_));
    }

    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status NtnServerImpl::UpdateSystemSelectionSpecifiers(ServerContext* context,
    const ::google::protobuf::Empty* request,
    satcomStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = SATCOM_API_LOCAL_JSON;
    std::string stateJsonPath = SATCOM_STATE_JSON;
    std::string subsystem = "INtnManager";
    std::string method = "updateSystemSelectionSpecifiers";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status NtnServerImpl::GetNtnState(ServerContext* context,
    const google::protobuf::Empty* request, satcomStub::GetNtnStateReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = SATCOM_API_LOCAL_JSON;
    std::string stateJsonPath = SATCOM_STATE_JSON;
    std::string subsystem = "INtnManager";
    std::string method = "getNtnState";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    response->set_state(
        static_cast<satcomStub::NtnState>(ntnState_));
    LOG(DEBUG, __FUNCTION__, " NtnState: ", static_cast<int>(ntnState_));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status NtnServerImpl::EnableCellularScan(ServerContext* context,
    const satcomStub::EnableCellularScanRequest* request,
    satcomStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = SATCOM_API_LOCAL_JSON;
    std::string stateJsonPath = SATCOM_STATE_JSON;
    std::string subsystem = "INtnManager";
    std::string method = "enableCellularScan";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    enableCellularScan_ = request->enable();
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    std::thread([this] {
        // Simulate cellular coverage availability
        std::this_thread::sleep_for(std::chrono::seconds(5));
        handleCellularCoverageAvailable("1");
    }).detach();

    return grpc::Status::OK;
}

grpc::Status NtnServerImpl::SetLocationFix(ServerContext* context,
    const satcomStub::SetLocationFixRequest* request,
    satcomStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = SATCOM_API_LOCAL_JSON;
    std::string stateJsonPath = SATCOM_STATE_JSON;
    std::string subsystem = "INtnManager";
    std::string method = "setLocationFix";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    if (ntnState_ == telux::satcom::NtnState::OUT_OF_SERVICE) {
        std::thread([this] {
           handleStateChangeRequest(NTN_IN_SERVICE);
        }).detach();
    }

    return grpc::Status::OK;
}

grpc::Status NtnServerImpl::LocationFixResponse(ServerContext* context,
    const satcomStub::LocationFixResponseRequest* request,
    satcomStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = SATCOM_API_LOCAL_JSON;
    std::string stateJsonPath = SATCOM_STATE_JSON;
    std::string subsystem = "INtnManager";
    std::string method = "locationFixResponse";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

void NtnServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == NTN_FILTER) {
        onEventUpdate(message.event());
    }
}

void NtnServerImpl::onEventUpdate(std::string event) {
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "String is ", token);
    if (token == "stateChange") {
        handleStateChangeRequest(event);
    } else if (token == "cellularCoverageAvailable") {
        handleCellularCoverageAvailable(event);
    } else if (token == "locationFixRequest") {
        handleLocationFixRequest(event);
    } else if (token == "incomingData") {
        handleIncomingData(event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}

void NtnServerImpl::handleStateChangeRequest(std::string event) {
    LOG(DEBUG, __FUNCTION__, " event:", event);
    int bandValue = 0;
    std::string state = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    try {
        int stateValue = std::stoi(state);
        LOG(DEBUG, __FUNCTION__, " stateValue:", stateValue);
        if (stateValue == 0) {
            ntnState_ = telux::satcom::NtnState::DISABLED;
            capabilities_ = 0;
            bandValue = 0;
            signalStrength_ = telux::satcom::SignalStrength::NONE;
        } else if (stateValue == 1) {
            ntnState_ = telux::satcom::NtnState::OUT_OF_SERVICE;
            capabilities_ = 0;
            bandValue = 0;
            signalStrength_ = telux::satcom::SignalStrength::NONE;
        } else if (stateValue == 2) {
            ntnState_ = telux::satcom::NtnState::IN_SERVICE;
            capabilities_ = 128;
            bandValue = 255;
            signalStrength_ = telux::satcom::SignalStrength::GREAT;
        } else {
            LOG(ERROR, __FUNCTION__, "Invalid state value");
            return;
        }
    } catch (const std::exception& ex) {
        LOG(ERROR, __FUNCTION__, "Exception occurred: ", ex.what());
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    // Post the event to EventService event queue
    ::eventService::EventResponse anyResponse;
    satcomStub::NtnStateEvent ntnStateEvent;
    ntnStateEvent.set_state(static_cast<satcomStub::NtnState>(ntnState_));
    ntnStateEvent.set_capabilities(capabilities_);
    ntnStateEvent.set_signal_strength(static_cast<satcomStub::SignalStrength>(signalStrength_));
    ntnStateEvent.set_band_value(bandValue);
    anyResponse.set_filter(NTN_FILTER);
    anyResponse.mutable_any()->PackFrom(ntnStateEvent);
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void NtnServerImpl::handleCellularCoverageAvailable(std::string event) {
    LOG(DEBUG, __FUNCTION__, " event:", event);
    std::string isAvailable = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    try {
        if (enableCellularScan_) {
            bool available = (isAvailable == "1");
            LOG(DEBUG, __FUNCTION__, " available:", available);
            // Post the event to EventService event queue
            ::eventService::EventResponse anyResponse;
            satcomStub::CellularCoverageAvailableEvent eventResponse;
            eventResponse.set_is_available(available);
            anyResponse.set_filter(NTN_FILTER);
            anyResponse.mutable_any()->PackFrom(eventResponse);
            auto& eventImpl = EventService::getInstance();
            eventImpl.updateEventQueue(anyResponse);
        }
    } catch (const std::exception& ex) {
        LOG(ERROR, __FUNCTION__, "Exception occurred: ", ex.what());
        return;
    }
}

void NtnServerImpl::handleLocationFixRequest(std::string event) {
    LOG(DEBUG, __FUNCTION__, " event:", event);
    std::string reqReason = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    try {
        telux::satcom::LocationFixRequestReason reason =
            static_cast<telux::satcom::LocationFixRequestReason>(std::stoi(reqReason));
        LOG(DEBUG, __FUNCTION__, " reason:", reason);
        // Handle the location fix request event
        ::eventService::EventResponse anyResponse;
        satcomStub::LocationFixRequestEvent eventResponse;
        eventResponse.set_req_reason(
            static_cast<satcomStub::LocationFixRequestReason>(reason));
        anyResponse.set_filter(NTN_FILTER);
        anyResponse.mutable_any()->PackFrom(eventResponse);
        auto& eventImpl = EventService::getInstance();
        eventImpl.updateEventQueue(anyResponse);
    } catch (const std::exception& ex) {
        LOG(ERROR, __FUNCTION__, "Exception occurred: ", ex.what());
        return;
    }
}

void NtnServerImpl::handleIncomingData(std::string event) {
    LOG(DEBUG, __FUNCTION__, " event:", event);
    std::string data = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    try {
        // Convert the data to a byte array
        std::vector<uint8_t> dataArray;
        size_t pos = 0;
        while ((pos = data.find(',')) != std::string::npos) {
            uint8_t byte = static_cast<uint8_t>(std::stoi(data.substr(0, pos)));
            dataArray.push_back(byte);
            data.erase(0, pos + 1);
        }
        uint8_t byte = static_cast<uint8_t>(std::stoi(data));
        dataArray.push_back(byte);

        // Post the event to EventService event queue
        ::eventService::EventResponse anyResponse;
        satcomStub::IncomingDataEvent eventResponse;
        for (uint8_t byte : dataArray) {
            eventResponse.add_data(byte);
        }
        anyResponse.set_filter(NTN_FILTER);
        anyResponse.mutable_any()->PackFrom(eventResponse);
        auto& eventImpl = EventService::getInstance();
        eventImpl.updateEventQueue(anyResponse);
    } catch (const std::exception& ex) {
        LOG(ERROR, __FUNCTION__, "Exception occurred: ", ex.what());
        return;
    }
}
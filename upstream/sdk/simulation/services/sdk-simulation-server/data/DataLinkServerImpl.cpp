/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>
#include <thread>
#include "DataLinkServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "event/EventService.hpp"

#define DATA_LINK_MANAGER_API_JSON "api/data/IDataLinkManager.json"
#define DATA_LINK_MANAGER_STATE_JSON "system-state/data/IDataLinkManagerState.json"

#define DATA_LINK "data_link"
#define DATA_LINK_SSR_FILTER "data_link_ssr"
#define ETH_DATA_LINK_STATE_CHANGE_FILTER "eth_data_link_state_change"

DataLinkServerImpl::DataLinkServerImpl()
    : serverEvent_(ServerEventManager::getInstance())
    , clientEvent_(EventService::getInstance()) {
    LOG(DEBUG, __FUNCTION__);
}

DataLinkServerImpl::~DataLinkServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status DataLinkServerImpl::registerDefaultIndications() {
    LOG(DEBUG, __FUNCTION__);

    auto status = serverEvent_.registerListener(shared_from_this(), DATA_LINK);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications failed");
        return status;
    }

    return status;
}

void DataLinkServerImpl::onSSREvent(telux::common::ServiceStatus srvStatus) {
    commonStub::GetServiceStatusReply ssrResp;
    ::eventService::EventResponse anyResponse;
    setResponse(srvStatus, &ssrResp);
    anyResponse.set_filter(DATA_LINK_SSR_FILTER);
    anyResponse.mutable_any()->PackFrom(ssrResp);
    clientEvent_.updateEventQueue(anyResponse);
}

void DataLinkServerImpl::notifyServiceStateChanged(telux::common::ServiceStatus srvStatus,
        std::string srvStatusStr) {
    LOG(DEBUG, __FUNCTION__, ":: Service status Changed to ", srvStatusStr);
    onSSREvent(srvStatus);
}

telux::common::ServiceStatus DataLinkServerImpl::getServiceStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    return serviceStatus_;
}

void DataLinkServerImpl::setServiceStatus(telux::common::ServiceStatus srvStatus) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (serviceStatus_ != srvStatus) {
        serviceStatus_ = srvStatus;
        std::string srvStrStatus = CommonUtils::mapServiceString(srvStatus);
        notifyServiceStateChanged(serviceStatus_, srvStrStatus);
    }
}

void DataLinkServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event) {
    if (event.filter() == DATA_LINK) {
        onEventUpdate(event.event());
    }
}

/* Get Notification for SSR */
void DataLinkServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__, ":: The thermal event: ", event);
    std::string token;

    /** INPUT-event:
     * (1) ssr SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
     * OUTPUT-token:
     * (1) ssr
     **/
    token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    /** INPUT-token:
     * (1) ssr
     * INPUT-event:
     * (1) SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
     **/
    handleEvent(token, event);
}

/** INPUT-token:
  * (1) ssr
  * INPUT-event:
  * (1) SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
 */
void DataLinkServerImpl::handleEvent(std::string token,std::string event) {
    LOG(DEBUG, __FUNCTION__, ":: The data link event type is: ", token,
            "The leftover string is: ", event);

    if (token == "ssr") {
        //INPUT-token: ssr
        //INPUT-event: SERVICE_AVAILABLE/SERVICE_UNAVAILABLE/SERVICE_FAILED
        handleSSREvent(event);
    } else {
        LOG(DEBUG, __FUNCTION__, ":: Invalid event ! Ignoring token: ",
                token, ", event: ", event);
    }
}

void DataLinkServerImpl::handleSSREvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__, ":: SSR event: ", eventParams);

    telux::common::ServiceStatus srvcStatus =
        telux::common::ServiceStatus::SERVICE_FAILED;
    if (eventParams == "SERVICE_AVAILABLE") {
        srvcStatus = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    } else if (eventParams == "SERVICE_UNAVAILABLE") {
        srvcStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    } else if (eventParams == "SERVICE_FAILED") {
        srvcStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    } else {
        // Ignore
        LOG(DEBUG, __FUNCTION__, ":: INVALID SSR event: ", eventParams);
        return;
    }

    setServiceStatus(srvcStatus);
}

grpc::Status DataLinkServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty *request, commonStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);

    Json::Value rootObj;

    auto status = registerDefaultIndications();
    if (status != telux::common::Status::SUCCESS) {
        return grpc::Status(grpc::StatusCode::CANCELLED,
                ":: Could not register indication with EventMgr");
    }

    std::string filePath = DATA_LINK_MANAGER_API_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IDataLinkManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IDataLinkManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus srvcStatus = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(srvcStatus));

    setServiceStatus(srvcStatus);

    return setResponse(srvcStatus,response);
}

grpc::Status DataLinkServerImpl::GetServiceStatus(ServerContext* context,
        const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::ServiceStatus srvStatus = getServiceStatus();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemStatus: ", static_cast<int>(srvStatus));

    return setResponse(srvStatus,response);
}

grpc::Status DataLinkServerImpl::setResponse(telux::common::ServiceStatus srvStatus,
        commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    Json::Value rootObj;
    int subSysDelay = rootObj["IDataLinkManager"]["IsSubsystemReadyDelay"].asInt();
    LOG(DEBUG, __FUNCTION__, ":: SubSystemDelay: ", subSysDelay);

    switch(srvStatus) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_AVAILABLE);
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_UNAVAILABLE);
            break;
        case telux::common::ServiceStatus::SERVICE_FAILED:
            response->set_service_status(commonStub::ServiceStatus::SERVICE_FAILED);
            break;
        default:
            LOG(ERROR, __FUNCTION__, ":: Invalid service status");
            return grpc::Status(grpc::StatusCode::CANCELLED, ":: set service status failed");
    }
    response->set_delay(subSysDelay);
    return grpc::Status::OK;
}

grpc::Status DataLinkServerImpl::SetEthDataLinkState(ServerContext* context,
    const dataStub::SetEthDatalinkStateRequest *request,
    dataStub::SetEthDatalinkStateReply* response) {

    LOG(DEBUG, __FUNCTION__);

    auto ethLinkState = request->eth_datalink_state().link_state();
    LOG(DEBUG, __FUNCTION__, ", ethLinkState: ", static_cast<int>(ethLinkState));

    std::string subsystem = "IDataLinkManager";
    std::string method = "setEthDataLinkState";

    Json::Value eth0LinkState, newEth0LinkState;
    JsonData data;
    std::string currLinkStateStr, newLinkStateStr;
    dataStub::OnEthDataLinkStateChangeReply indication;
    ::eventService::EventResponse anyResponse;

    telux::common::ErrorCode error =
        CommonUtils::readJsonData(DATA_LINK_MANAGER_API_JSON, DATA_LINK_MANAGER_STATE_JSON,
                subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error != telux::common::ErrorCode::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, ", ignoring state update");
        response->set_error(static_cast<commonStub::ErrorCode>(data.error));
        return grpc::Status::OK;
    }

    if (ethLinkState == ::dataStub::LinkStateEnum_LinkState_UP) {
        newLinkStateStr = "UP";
    } else {
        newLinkStateStr = "DOWN";
    }

    currLinkStateStr = data.stateRootObj[subsystem]["eth0Config"]["ethLinkState"].asString();

    if (newLinkStateStr == currLinkStateStr) {
        LOG(DEBUG, __FUNCTION__, ", ignoring redundant notifications");
        response->set_error(static_cast<commonStub::ErrorCode>(data.error));
        return grpc::Status::OK;
    }

    newEth0LinkState["ethLinkState"] = newLinkStateStr;
    data.stateRootObj[subsystem]["eth0Config"] = newEth0LinkState;

    // Updae eth0 datalink state
    JsonParser::writeToJsonFile(data.stateRootObj, DATA_LINK_MANAGER_STATE_JSON);

    // Notify about data link state change
    anyResponse.set_filter(ETH_DATA_LINK_STATE_CHANGE_FILTER);
    indication.mutable_eth_datalink_state()->set_link_state(
            request->eth_datalink_state().link_state());
    anyResponse.mutable_any()->PackFrom(indication);
    clientEvent_.updateEventQueue(anyResponse);

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}


grpc::Status DataLinkServerImpl::GetEthDataLinkState(ServerContext* context,
    const google::protobuf::Empty* request,
    dataStub::GetEthDataLinkStateReply* response) {

    LOG(DEBUG, __FUNCTION__);

    std::string subsystem = "IDataLinkManager";
    std::string method = "getEthDataLinkState";

    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(DATA_LINK_MANAGER_API_JSON, DATA_LINK_MANAGER_STATE_JSON,
                                  subsystem, method, data);

    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "Failed to read JSON data");
        response->set_error(static_cast<commonStub::ErrorCode>(data.error));
        return grpc::Status::OK;
    }

    auto &ethConfig = data.stateRootObj[subsystem]["eth0Config"];
    if (!ethConfig.isMember("ethLinkState")) {
        LOG(ERROR, __FUNCTION__, "Missing ethLinkState in JSON");
        response->set_error(static_cast<commonStub::ErrorCode>(data.error));
        return grpc::Status::OK;
    }

    std::string linkStateStr = ethConfig["ethLinkState"].asString();
    dataStub::LinkStateEnum_LinkState linkState;

    if (linkStateStr == "UP") {
        linkState = dataStub::LinkStateEnum_LinkState_UP;
    } else if (linkStateStr == "DOWN") {
        linkState = dataStub::LinkStateEnum_LinkState_DOWN;
    } else {
        linkState = dataStub::LinkStateEnum_LinkState_UNKNOWN;
        LOG(ERROR, __FUNCTION__, "Invalid ethLinkState value in JSON: ", linkStateStr);
        response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    }
    LOG(DEBUG,__FUNCTION__,"linkState: ",linkState);
    response->mutable_eth_datalink_state()->set_link_state(linkState);
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status DataLinkServerImpl::GetEthCapability(ServerContext* context,
    const ::google::protobuf::Empty* request,
    dataStub::GetEthCapabilityReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string subsystem = "IDataLinkManager";
    std::string method = "getEthCapability";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(DATA_LINK_MANAGER_API_JSON, DATA_LINK_MANAGER_STATE_JSON,
            subsystem, method, data);
    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " JSON read failed");
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    const Json::Value &ethModesJson = data.stateRootObj[subsystem]["eth0Config"]["ethModes"];
    for (const auto &modeStr : ethModesJson) {
        std::string mode = modeStr.asString();
        if (mode == "USXGMII_10G") {
            response->mutable_capability()->add_eth_modes(dataStub::EthModeEnum::EthModeEnum_USXGMII_10G);
        } else if (mode == "USXGMII_5G") {
            response->mutable_capability()->add_eth_modes(dataStub::EthModeEnum::EthModeEnum_USXGMII_5G);
        } else if (mode == "SGMII_1G") {
            response->mutable_capability()->add_eth_modes(dataStub::EthModeEnum::EthModeEnum_SGMII_1G);
        } else {
            LOG(WARNING, __FUNCTION__, " Unknown eth mode: ", mode);
        }
    }
     std::string statusStr = data.apiRootObj[subsystem][method]["status"].asString();
    if (statusStr == "SUCCESS") {
        response->set_status(commonStub::Status::SUCCESS);
    } else {
        response->set_status(commonStub::Status::FAILED);
    }

    LOG(DEBUG, __FUNCTION__, " ethernet capability: ", ethModesJson.toStyledString());

    return grpc::Status::OK;
}

grpc::Status DataLinkServerImpl::SetLocalEthOperatingMode(
    ServerContext* context,
    const dataStub::SetLocalEthOperatingModeRequest* request,
    dataStub::SetLocalEthOperatingModeReply* response) {

    LOG(DEBUG, __FUNCTION__);

    int mode = request->eth_mode();
    std::string modeStr;

    switch (mode) {
        case 0: modeStr = "USXGMII_10G"; break;
        case 1: modeStr = "USXGMII_5G"; break;
        case 2: modeStr = "USXGMII_2_5G"; break;
        case 3: modeStr = "USXGMII_1G"; break;
        case 4: modeStr = "USXGMII_100M"; break;
        case 5: modeStr = "USXGMII_10M"; break;
        case 6: modeStr = "SGMII_2_5G"; break;
        case 7: modeStr = "SGMII_1G"; break;
        case 8: modeStr = "SGMII_100M"; break;
        default: modeStr = "UNKNOWN"; break;
    }

    std::cout << " *** Set local Eth operating mode request sent\n";
    std::cout << " Simulating mode change to: " << modeStr << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    JsonData data;
    telux::common::ErrorCode error = CommonUtils::readJsonData(
        DATA_LINK_MANAGER_API_JSON, DATA_LINK_MANAGER_STATE_JSON,
        "IDataLinkManager", "setLocalEthOperatingMode", data);

    if (error == telux::common::ErrorCode::SUCCESS) {
        data.stateRootObj["IDataLinkManager"]["eth0Config"]["ethOperatingMode"] = modeStr;
        JsonParser::writeToJsonFile(data.stateRootObj, DATA_LINK_MANAGER_STATE_JSON);
    }
    else {
        LOG(WARNING, __FUNCTION__, " Failed to read/write JSON state");
    }


    std::cout << "NOTIFICATION:  ** DataLinkManager onEthModeChangeTransactionStatus **\n";
    std::cout << " " << modeStr << " ,status :" << mode << " COMPLETED\n";
    std::cout << " *** Set local Eth operating mode request completed\n";

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_status(dataStub::ModeChangeStatusEnum::ModeChangeStatusEnum_COMPLETED);

    return grpc::Status::OK;
}

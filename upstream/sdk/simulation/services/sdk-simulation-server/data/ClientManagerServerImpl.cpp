/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include <telux/common/DeviceConfig.hpp>
#include "ClientManagerServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "event/EventService.hpp"
#include "event/ServerEventManager.hpp"

#include <unordered_set>
#include <thread>
#include <chrono>


#define CLIENT_MANAGER_API_JSON   "api/data/IClientManager.json"
#define CLIENT_MANAGER_STATE_JSON "system-state/data/IClientManagerState.json"
#define CLIENT_MANAGER_FILTER     "client_manager"
#define CLIENT_MANAGER            "IClientManager"


ClientManagerServerImpl::ClientManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

ClientManagerServerImpl::~ClientManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
     if (taskQ_) {
        taskQ_ = nullptr;
    }
}

grpc::Status ClientManagerServerImpl::InitService(ServerContext* context,
    const ::dataStub::InitRequest* request, ::dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = CLIENT_MANAGER_API_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IClientManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IClientManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    std::vector<std::string> filters = {CLIENT_MANAGER_FILTER};
    auto &serverEventManager = ServerEventManager::getInstance();
    serverEventManager.registerListener(shared_from_this(), filters);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    subSystemStatus_ = status;

    return grpc::Status::OK;

}


bool ClientManagerServerImpl::isMonitoringEnabled() {
    std::string subsystem = "IDataSettingsManager";
    std::string method = "isDeviceDataUsageMonitoringEnabled";

    LOG(DEBUG, __FUNCTION__);
    Json::Value root;
    telux::common::ErrorCode err = JsonParser::readFromJsonFile(root,
        "system-state/data/IDataSettingsManagerState.json");

    if (err == telux::common::ErrorCode::SUCCESS) {
        bool enabled = root["IDataSettingsManager"]
            ["isDeviceDataUsageMonitoringEnabled"]["enabled"].asBool();
        LOG(DEBUG, __FUNCTION__, "isDeviceDataUsageMonitoringEnabled: ", enabled);
        return enabled;
    } else {
        LOG(ERROR, __FUNCTION__, "Error reading JSON file: ", static_cast<int>(err));
        return false;
    }
}



grpc::Status ClientManagerServerImpl::GetDeviceDataUsageStats(
    ServerContext* context,
    const ::google::protobuf::Empty* request,
    dataStub::GetDeviceDataUsageStatsResponse* response) {

    LOG(DEBUG, __FUNCTION__);
    bool enabled = isMonitoringEnabled();

    if (!enabled) {
        LOG(DEBUG, __FUNCTION__,"isMonitoringEnabled disabled");
        response->set_error(commonStub::ErrorCode::INVALID_STATE);
        return grpc::Status(grpc::StatusCode::INTERNAL,
            "isDeviceDataUsageMonitoringEnabled Disabled");
    }
    else {
        LOG(DEBUG, __FUNCTION__,"isMonitoringEnabled success");
    }

    Json::Value root;
    if (JsonParser::readFromJsonFile(root, CLIENT_MANAGER_STATE_JSON)
            == telux::common::ErrorCode::SUCCESS) {
        const Json::Value &stats = root["IClientManager"]["deviceUsageStats"];
        for (Json::ArrayIndex i = 0; i < stats.size(); ++i) {
            const Json::Value &entry = stats[i];
            std::string suffix = std::to_string(i + 1);

            std::string macKey = "macAddress";
            std::string rxKey = "bytesRx";
            std::string txKey = "bytesTx";

            auto *deviceUsage = response->add_usage_stats();
            deviceUsage->set_mac_address(entry[macKey].asString());
            deviceUsage->mutable_usage()->set_bytes_rx(entry[rxKey].asUInt64());
            deviceUsage->mutable_usage()->set_bytes_tx(entry[txKey].asUInt64());
        }

        response->set_error(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    } else {
        response->set_error(commonStub::ErrorCode::INTERNAL_ERROR);
    }

    return grpc::Status::OK;
}



grpc::Status ClientManagerServerImpl::ResetDataUsageStats(
    ServerContext* context,
    const dataStub::ResetDataUsageStatsRequest* request,
    dataStub::ResetDataUsageStatsResponse* response) {

    LOG(DEBUG, __FUNCTION__);

    Json::Value root;
    std::string filePath = CLIENT_MANAGER_STATE_JSON;

    if (JsonParser::readFromJsonFile(root, filePath) != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Failed to read existing stats from JSON");
        response->set_error(commonStub::ErrorCode::INTERNAL_ERROR);
         return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    Json::Value &stats = root["IClientManager"]["deviceUsageStats"];
    for (auto &entry : stats) {
         entry["bytesRx"] = 0;
        entry["bytesTx"] = 0;
    }

    if (JsonParser::writeToJsonFile(root, filePath) != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Failed to write reset stats to JSON");
        response->set_error(commonStub::ErrorCode::INTERNAL_ERROR);
         return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    response->set_error(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    return grpc::Status::OK;
}


void ClientManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
     LOG(DEBUG, __FUNCTION__, "Event Called");
    if (message.filter() == CLIENT_MANAGER_FILTER) {
        onEventUpdate(message.event());
    }
}

void ClientManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__, "String is ", event);
    std::string token = EventParserUtil::getNextToken(event, " ");
    if (token == "deviceDataUsageStatsUpdate") {
        handleDeviceDataUsageStatsUpdate(event);
    } else if (token == "deviceDataUsageReset") {
        handleDeviceDataUsageReset(event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}



std::vector<std::string> ClientManagerServerImpl::splitBySpace(const std::string &input) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::string> tokens;
    std::string remaining = input;

    while (!remaining.empty()) {
        std::string token = EventParserUtil::getNextToken(remaining, " ");
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    LOG(DEBUG, __FUNCTION__, "Found ", tokens.size(), " tokens");
    return tokens;
}


void ClientManagerServerImpl::handleDeviceDataUsageReset(std::string event) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::string> tokens = splitBySpace(event);
    if (tokens.size() < 2) {
        LOG(ERROR, __FUNCTION__, "Invalid event format");
        return;
    }
    std::vector<std::pair<std::string, std::string>> devices = {
        {tokens[0], tokens[1]},
    };
    Json::Value root;
    std::string filePath = CLIENT_MANAGER_STATE_JSON;
    if (JsonParser::readFromJsonFile(root, filePath) != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "Failed to read existing stats from JSON");
        return;
    }
    Json::Value &stats = root["IClientManager"]["deviceUsageStats"];
    std::unordered_map<std::string, std::tuple<uint64_t, uint64_t, std::string>> retainedStats;
    try{
        for (const auto &device : devices) {
            const std::string &mac = device.first;
            const std::string &reason = device.second;
        for (Json::Value::ArrayIndex i = 0; i < stats.size(); i++) {
            if (stats[i].isMember("macAddress") && stats[i]["macAddress"].asString() == mac) {
                if (reason == "DEVICE_DISCONNECTED") {
                    uint64_t retainedRx = stats[i]["bytesRx"].asUInt64();
                    uint64_t retainedTx = stats[i]["bytesTx"].asUInt64();
                    retainedStats[mac] = {retainedRx, retainedTx, reason};
                    LOG(DEBUG, __FUNCTION__, "Resetting stats for MAC: ", mac,
                        ", bytesRx = ", retainedRx, ", bytesTx = ", retainedTx);
                    ::dataStub::DeviceDataUsageResetEvent resetEvent;
                    resetEvent.set_mac_address(mac);
                    resetEvent.set_bytes_rx(retainedRx);
                    resetEvent.set_bytes_tx(retainedTx);
                    resetEvent.set_reason(reason);
                    ::eventService::EventResponse response;
                    response.set_filter(CLIENT_MANAGER_FILTER);
                    response.mutable_any()->PackFrom(resetEvent);
                    EventService::getInstance().updateEventQueue(response);
                } else {
                    // Send notifications for all devices
                    for (Json::Value::ArrayIndex j = 0; j < stats.size(); j++) {
                        std::string deviceMac = stats[j]["macAddress"].asString();
                        uint64_t bytesRx = stats[j]["bytesRx"].asUInt64();
                        uint64_t bytesTx = stats[j]["bytesTx"].asUInt64();
                        ::dataStub::DeviceDataUsageResetEvent resetEvent;
                        resetEvent.set_mac_address(deviceMac);
                        resetEvent.set_bytes_rx(bytesRx);
                        resetEvent.set_bytes_tx(bytesTx);
                        resetEvent.set_reason(reason);
                        ::eventService::EventResponse response;
                        response.set_filter(CLIENT_MANAGER_FILTER);
                        response.mutable_any()->PackFrom(resetEvent);
                        EventService::getInstance().updateEventQueue(response);
                    }
                    // Set device stats to 0
                    for (Json::Value::ArrayIndex j = 0; j < stats.size(); j++) {
                        stats[j]["bytesRx"] = 0;
                        stats[j]["bytesTx"] = 0;
                    }
                }
                break;
            }
        }
    }
     // Remove device from list if reason is DEVICE_DISCONNECTED
    for (Json::Value::ArrayIndex i = stats.size(); i-- > 0;) {
        if (!stats[i].isMember("macAddress")) continue;
        std::string mac = stats[i]["macAddress"].asString();
        auto it = retainedStats.find(mac);
        if (it != retainedStats.end()) {
            std::string reason = std::get<2>(it->second);
            if (reason == "DEVICE_DISCONNECTED") {
                Json::Value updatedStats;
                for (const auto& stat : stats) {
                    if (stat["macAddress"].asString() != mac) {
                        updatedStats.append(stat);
                    }
                }
                root["IClientManager"]["deviceUsageStats"] = updatedStats;
            }
        }
    }
    } catch(const std::exception &e){
         LOG(ERROR, __FUNCTION__, "Exception while processing device stats");
    }
    // Clear the map to free memory
    retainedStats.clear();

    // Write updated stats to file
    if (JsonParser::writeToJsonFile(root, filePath) != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "Failed to write updated stats to JSON");
    } else {
        LOG(DEBUG, __FUNCTION__, "Device usage stats reset successfully.");
    }
}


void ClientManagerServerImpl::handleDeviceDataUsageStatsUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__);
    bool enabled = isMonitoringEnabled();
    if (!enabled) {
        LOG(DEBUG, __FUNCTION__,"isMonitoringEnabled failed");
        return;
    }
    else {
        LOG(DEBUG, __FUNCTION__,"isMonitoringEnabled success");
    }

    try {
        std::string macAddress = EventParserUtil::getNextToken(event, " ");
        std::string rxToken = EventParserUtil::getNextToken(event, " ");
        std::string txToken = EventParserUtil::getNextToken(event, " ");

        if (macAddress.empty() || rxToken.empty() || txToken.empty()) {
            LOG(ERROR, __FUNCTION__, "Invalid or missing input tokens.");
            return;
        }

        int bytesRx = 0, bytesTx = 0;
        try {
            bytesRx = std::stoi(rxToken);
            bytesTx = std::stoi(txToken);
        } catch (const std::exception &e) {
            LOG(ERROR, __FUNCTION__, "Invalid RX/TX values: ", e.what());
            return;
        }

        Json::Value root;
        if (JsonParser::readFromJsonFile(root, CLIENT_MANAGER_STATE_JSON) != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, "Failed to read existing stats from JSON");
            return;
        }

        Json::Value &stats = root["IClientManager"]["deviceUsageStats"];
        bool updated = false;

        for (Json::Value &entry : stats) {
            if (entry["macAddress"].asString() == macAddress) {
                entry["bytesRx"] = bytesRx;
                entry["bytesTx"] = bytesTx;
                updated = true;
                break;
            }
        }

        if (!updated) {
            Json::Value newEntry;
            newEntry["macAddress"] = macAddress;
            newEntry["bytesRx"] = bytesRx;
            newEntry["bytesTx"] = bytesTx;
            stats.append(newEntry);
        }
        if (JsonParser::writeToJsonFile(root, CLIENT_MANAGER_STATE_JSON) != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, "Failed to write updated stats to JSON");
        } else {
            LOG(DEBUG, __FUNCTION__, "Device usage stats updated successfully.");
        }
    } catch (const std::exception &e) {
        LOG(ERROR, __FUNCTION__, "Exception occurred: ", e.what());
    }
}
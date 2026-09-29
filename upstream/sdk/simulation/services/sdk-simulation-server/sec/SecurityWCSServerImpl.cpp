/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "libs/common/Logger.hpp"
#include "libs/common/CommonUtils.hpp"

#include "SecurityWCSServerImpl.hpp"

SecurityWCSServerImpl::SecurityWCSServerImpl()
    : serverEvent_(ServerEventManager::getInstance())
    , clientEvent_(EventService::getInstance()) {
}

SecurityWCSServerImpl::~SecurityWCSServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

/*
 * Server side initialization.
 */
grpc::Status SecurityWCSServerImpl::Init(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request, ::securityStub::InitInfo* response) {

    uint32_t delay = 0;
    std::string srvStatus;

    telux::common::ErrorCode ec;
    telux::common::Status status;

    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, WCS_API_JSON_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", WCS_API_JSON_FILE);
            response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    delay = apiConfigJsonRoot_["IWiFiSecurityManager"]["IsSubsystemReadyDelay"].asInt();

    srvStatus = apiConfigJsonRoot_["IWiFiSecurityManager"]["IsSubsystemReady"].asString();
    if ((srvStatus == "SERVICE_AVAILABLE") && (!isServiceInitialized_)) {
        status = serverEvent_.registerListener(shared_from_this(), WCS_FILTER);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't register with ServerEventManager");
            response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    if (srvStatus == "SERVICE_AVAILABLE") {
        response->set_service_status(commonStub::ServiceStatus::SERVICE_AVAILABLE);
    } else if (srvStatus == "SERVICE_UNAVAILABLE") {
        response->set_service_status(commonStub::ServiceStatus::SERVICE_UNAVAILABLE);
    } else if (srvStatus == "SERVICE_FAILED") {
        response->set_service_status(commonStub::ServiceStatus::SERVICE_FAILED);
    } else {
    }

    response->set_ss_ready_delay(delay);
    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    clientsCount_++;
    isServiceInitialized_ = true;
    return grpc::Status::OK;
}

/*
 * Server side deinitialization.
 */
grpc::Status SecurityWCSServerImpl::DeInit(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request, ::commonStub::ErrorCodeMsg* response) {

    clientsCount_--;
    if (!clientsCount_) {
        serverEvent_.deregisterListener(shared_from_this(), WCS_FILTER);
        isServiceInitialized_ = false;

        // Empty all entries in WCS_DATABASE_FILE
        Json::Value rootObj;
        rootObj[ACCESS_POINTS] = Json::arrayValue;
        JsonParser::writeToJsonFile(rootObj, WCS_DATABASE_FILE);
    }

    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

/*
 * This is equivalent to calling ssg_connsec_wcs_connect() that indicates, now reports
 * will start coming to the application.
 */
grpc::Status SecurityWCSServerImpl::RegisterClientForReport(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request, ::commonStub::ErrorCodeMsg* response) {
    std::string ecStr = "";
    telux::common::ErrorCode ec;
    ecStr = apiConfigJsonRoot_["IWiFiSecurityManager"]["registerListener"]["error"].asString();
    ec = CommonUtils::mapErrorCode(ecStr);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        response->set_ec(static_cast<commonStub::ErrorCode>(ec));
        return grpc::Status::OK;
    }

    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    return grpc::Status::OK;
}

/*
 * This is equivalent to calling ssg_connsec_wcs_disconnect() that indicates.
 */
grpc::Status SecurityWCSServerImpl::DeregisterClientForReport(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request, ::commonStub::ErrorCodeMsg* response) {

    std::string ecStr = "";
    telux::common::ErrorCode ec;
    ecStr = apiConfigJsonRoot_["IWiFiSecurityManager"]["deregisterListener"]["error"].asString();
    ec = CommonUtils::mapErrorCode(ecStr);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        response->set_ec(static_cast<commonStub::ErrorCode>(ec));
        return grpc::Status::OK;
    }
    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    return grpc::Status::OK;
}

/*
 * Retrive a list of the saved trusted access points from the database.
 */
grpc::Status SecurityWCSServerImpl::GetTrustedApList(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request, ::securityStub::TrustedAPList* response) {

    int apCount = 0;
    telux::common::ErrorCode ec;

    Json::Value rootObj;

    std::string ecStr = "";
    ecStr = apiConfigJsonRoot_["IWiFiSecurityManager"]["getTrustedApList"]["error"].asString();
    ec = CommonUtils::mapErrorCode(ecStr);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        response->set_ec(static_cast<commonStub::ErrorCode>(ec));
        return grpc::Status::OK;
    }

    ec = JsonParser::readFromJsonFile(rootObj, WCS_DATABASE_FILE);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't open ", WCS_DATABASE_FILE);
        return grpc::Status::OK;
    }

    apCount = rootObj[ACCESS_POINTS].size();

    for (int x = 0; x < apCount; x++) {
        ::securityStub::ApInfo &apInfo = *response->add_ap_list();
        apInfo.set_ssid(rootObj[ACCESS_POINTS][x]["ssid"].asString());
        apInfo.set_bssid(rootObj[ACCESS_POINTS][x]["bssid"].asString());
    }

    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

/*
 * Save the user trusted access point in the database.
 */
grpc::Status SecurityWCSServerImpl::SetTrustedAp(::grpc::ServerContext* context,
    const ::securityStub::IsTrustedUserResponse* request, ::commonStub::ErrorCodeMsg* response) {

    int entryIndex = 0;

    Json::Value newAp;
    Json::Value rootObj;

    if (!request->is_trusted()) {
        /* If user distrusted, bail out early, don't modify database */
        response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
        return grpc::Status::OK;
    }

    entryIndex = rootObj[ACCESS_POINTS].size();

    newAp["ssid"] = request->ssid();
    newAp["bssid"] = request->bssid();

    rootObj[ACCESS_POINTS][entryIndex] = newAp;
    JsonParser::writeToJsonFile(rootObj, WCS_DATABASE_FILE);

    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

/*
 * Deletes given access point from the database.
 */
grpc::Status SecurityWCSServerImpl::RemoveApFromTrustedList(::grpc::ServerContext* context,
    const ::securityStub::ApInfo* request, ::commonStub::ErrorCodeMsg* response) {

    int x = 0;
    int apCount = 0;
    bool apToRemoveFound = false;
    telux::common::ErrorCode ec;
    std::string ecStr = "";

    Json::Value curObj;
    Json::Value newObj;

    ecStr = apiConfigJsonRoot_["IWiFiSecurityManager"]["removeApFromTrustedList"]["error"].asString();
    ec = CommonUtils::mapErrorCode(ecStr);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        response->set_ec(static_cast<commonStub::ErrorCode>(ec));
        return grpc::Status::OK;
    }

    ec = JsonParser::readFromJsonFile(curObj, WCS_DATABASE_FILE);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't open ", WCS_DATABASE_FILE);
        response->set_ec(commonStub::ErrorCode::FILE_NOT_FOUND);
        return grpc::Status::OK;
    }

    apCount = curObj[ACCESS_POINTS].size();

    for (int y = 0; y < apCount; y++) {
        if ((request->ssid() == curObj[ACCESS_POINTS][y]["ssid"].asString())
            && (request->bssid() == curObj[ACCESS_POINTS][y]["bssid"].asString())) {
                /* skip adding this AP to the database */
                apToRemoveFound = true;
                continue;
        }
        newObj[ACCESS_POINTS][x]["ssid"] = curObj[ACCESS_POINTS][y]["ssid"].asString();
        newObj[ACCESS_POINTS][x]["bssid"] = curObj[ACCESS_POINTS][y]["bssid"].asString();
        x++;
    }

    if (apToRemoveFound) {
        JsonParser::writeToJsonFile(newObj, WCS_DATABASE_FILE);
        response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    } else {
        LOG(ERROR, __FUNCTION__, " can't remove AP");
        response->set_ec(commonStub::ErrorCode::GENERIC_FAILURE);
    }

    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

/*
 * Handle events injected externally by the user.
 */
void SecurityWCSServerImpl::onEventUpdate(::eventService::UnsolicitedEvent usrEvent) {
    LOG(DEBUG,__FUNCTION__);

    std::string event;
    std::string token;

    if (usrEvent.filter() != WCS_FILTER) {
        return;
    }

    event = usrEvent.event();
    token = EventParserUtil::getNextToken(event, WCS_DEFAULT_DELIMITER);

    if (token == "ssr") {
        /* SSR is handled at the client library side (qmi service error, server crash etc.),
        * therefore process it further */
        return handleSSREvent(event);
    }

    if (!clientsCount_) {
        /* ssg_connsec_wcs_connect() was never called, therefore, injecting
         * event itself is wrong */
        LOG(ERROR, __FUNCTION__, " no listener registered");
        return;
    }

    if (token == "sec_report") {
        handleSecurityReportEvent(event);
    } else if (token == "deauth_attack") {
        handleDeauthEvent(event);
    } else if (token == "is_trusted_ap") {
        handleIsTrustedAP(event);
    } else {
    }
}

/*
 * Receive service status injected by the user. Translate it and dispatch for
 * delivery to the application.
 *
 * telsdk_event_injector -f wcs -e ssr SERVICE_UNAVAILABLE
 */
void SecurityWCSServerImpl::handleSSREvent(std::string eventParams) {

    ::securityStub::WCSServiceStatus newServiceState{};
    ::eventService::EventResponse anyResponse{};

    if (eventParams == "SERVICE_AVAILABLE") {
        newServiceState.set_service_status(commonStub::ServiceStatus::SERVICE_AVAILABLE);
    } else if (eventParams == "SERVICE_UNAVAILABLE" || eventParams == "SERVICE_FAILED") {
        newServiceState.set_service_status(commonStub::ServiceStatus::SERVICE_UNAVAILABLE);

        // Empty all entries in WCS_DATABASE_FILE
        Json::Value rootObj;
        rootObj[ACCESS_POINTS] = Json::arrayValue;
        JsonParser::writeToJsonFile(rootObj, WCS_DATABASE_FILE);
    } else {
        LOG(ERROR, __FUNCTION__, " invalid parameters: ", eventParams);
        return;
    }

    anyResponse.set_filter(WCS_FILTER);
    anyResponse.mutable_any()->PackFrom(newServiceState);
    clientEvent_.updateEventQueue(anyResponse);
}

/*
 * Receive report injected by the user. Translate it and dispatch for application.
 *
 * telsdk_event_injector -f wcs -e sec_report ssid wifiname bssid 02:13:37:a9:50:09
 * is_connected false is_open true ml_algo_analysis_threat_score 100 ml_analysis_result 3
 * summoning_analysis_result 2
 */
void SecurityWCSServerImpl::handleSecurityReportEvent(std::string eventParams) {

    std::string ssid;
    std::string bssid;
    bool isConnected = false;
    bool isOpen = false;
    uint32_t mlAlgoThreatScore = 0;
    int32_t mlAlgoAnalysisResult = 0;
    int32_t summoningAnalysisResult = 0;

    std::string token;
    ::securityStub::WCSReport report{};
    ::eventService::EventResponse anyResponse{};

    while (1) {
        token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
        if (token.empty()) {
            break;
        }
        if (token == "ssid") {
            token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
            ssid = token;
        } else if (token == "bssid") {
            token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
            bssid = token;
        } else if (token == "is_connected") {
            token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
            if (token == "true") {
                isConnected = true;
            }
        } else if (token == "is_open") {
            token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
            if (token == "true") {
                isOpen = true;
            }
        } else if (token == "ml_algo_analysis_threat_score") {
            token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
            try {
                mlAlgoThreatScore = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret score ", token);
                return ;
            }
        } else if (token == "ml_algo_analysis_result") {
            token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
            try {
                mlAlgoAnalysisResult = std::stoi(token);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret ml algo analysis result ", token);
                return ;
            }
        } else if (token == "summoning_analysis_result") {
            token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
            try {
                summoningAnalysisResult = std::stoi(token);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret summoning analysis result ", token);
                return ;
            }
        } else {
        }
    }

    report.set_ssid(ssid);
    report.set_bssid(bssid);
    report.set_is_connected_to_ap(isConnected);
    report.set_is_open_ap(isOpen);
    report.set_mlalgo_threat_score(mlAlgoThreatScore);
    report.set_mlalgo_analysis_result(mlAlgoAnalysisResult);
    report.set_summoning_analysis_result(summoningAnalysisResult);

    anyResponse.set_filter(WCS_FILTER);
    anyResponse.mutable_any()->PackFrom(report);
    clientEvent_.updateEventQueue(anyResponse);
}

/*
 * Receive deauth attack information injected by the user. Translate it and dispatch for
 * delivery to the application.
 *
 * telsdk_event_injector -f wcs -e deauth_attack deauth_reason 7 ap_init_disconnect true score 25
 */
void SecurityWCSServerImpl::handleDeauthEvent(std::string eventParams) {

    int32_t deauthReason = 0;
    bool apInitiateDisconnect = false;
    uint32_t threatScore = 0;

    std::string token;
    ::securityStub::DeauthenticationInfo deauthAttack{};
    ::eventService::EventResponse anyResponse{};

    while (1) {
        token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
        if (token.empty()) {
            break;
        }
        if (token == "deauth_reason") {
            token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
            try {
                deauthReason = std::stoi(token);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret deauth reason ", token);
                return;
            }
        } else if (token == "ap_init_disconnect") {
            token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
            if (token == "true") {
                apInitiateDisconnect = true;
            }
        } else if (token == "score") {
            token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
            try {
                threatScore = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret score ", token);
                return ;
            }
        } else {
        }
    }

    deauthAttack.set_deauthentication_reason(deauthReason);
    deauthAttack.set_did_ap_initiate_disconnect(apInitiateDisconnect);
    deauthAttack.set_threat_score(threatScore);

    anyResponse.set_filter(WCS_FILTER);
    anyResponse.mutable_any()->PackFrom(deauthAttack);
    clientEvent_.updateEventQueue(anyResponse);
}

/*
 * Receive report injected by the user. Translate it and dispatch for application.
 * Note - if the exact same AP is injected more then once then this itself is wrong.
 * SSG WCS checks if the AP has same or different finger print. If different then
 * only isTrusted() callback is invoked. Since we are saving only ssid and bssid and
 * not comparing any other parameter, user is expected to not inject same AP again.
 *
 * telsdk_event_injector -f wcs -e is_trusted_ap ssid wifiname bssid 02:13:37:a9:50:09
 */
void SecurityWCSServerImpl::handleIsTrustedAP(std::string eventParams) {
    std::string ssid;
    std::string bssid;

    std::string token;
    ::securityStub::ApInfo apInfo{};
    ::eventService::EventResponse anyResponse{};

    while (1) {
        token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
        if (token.empty()) {
            break;
        }
        if (token == "ssid") {
            token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
            ssid = token;
        } else if (token == "bssid") {
            token = EventParserUtil::getNextToken(eventParams, WCS_DEFAULT_DELIMITER);
            bssid = token;
        } else {
        }
    }

    apInfo.set_ssid(ssid);
    apInfo.set_bssid(bssid);

    anyResponse.set_filter(WCS_FILTER);
    anyResponse.mutable_any()->PackFrom(apInfo);
    clientEvent_.updateEventQueue(anyResponse);
}

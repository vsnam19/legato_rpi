/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include <telux/sec/CellularSecurityManager.hpp>

#include "libs/common/Logger.hpp"
#include "libs/common/CommonUtils.hpp"

#include "SecurityCCSServerImpl.hpp"

SecurityCCSServerImpl::SecurityCCSServerImpl()
    : serverEvent_(ServerEventManager::getInstance())
    , clientEvent_(EventService::getInstance()) {
}

SecurityCCSServerImpl::~SecurityCCSServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status SecurityCCSServerImpl::Init(::grpc::ServerContext* context,
    const ::securityStub::CCSConnectInfo* request, ::commonStub::ErrorCodeMsg* response) {

    telux::common::ErrorCode ec;
    telux::common::Status status;

    if (isServiceInitialized_) {
        clientsCount_++;
        response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
        return grpc::Status::OK;
    }

    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, CCS_API_JSON_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", CCS_API_JSON_FILE);
            response->set_ec(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    status = serverEvent_.registerListener(shared_from_this(), CCS_FILTER);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't register with ServerEventManager");
        response->set_ec(commonStub::ErrorCode::SYSTEM_ERR);
        return grpc::Status::OK;
    }

    curSessionStats_ = {};
    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    clientsCount_++;
    isServiceInitialized_ = true;

    return grpc::Status::OK;
}

grpc::Status SecurityCCSServerImpl::DeInit(::grpc::ServerContext* context,
    const ::securityStub::CCSDisconnectInfo* request, ::commonStub::ErrorCodeMsg* response) {

    clientsCount_--;
    if (!clientsCount_) {
        serverEvent_.deregisterListener(shared_from_this(), CCS_FILTER);
        isServiceInitialized_ = false;
        curSessionStats_ = {};
    }

    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

grpc::Status SecurityCCSServerImpl::GetCurrentSessionStats(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request, ::securityStub::SessionStats* response) {
    std::string ecStr = "";
    telux::common::ErrorCode ec{};

    ecStr = apiConfigJsonRoot_["ICellularSecurityManager"]["getCurrentSessionStats"][
        "error"].asString();
    ec = CommonUtils::mapErrorCode(ecStr);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        response->set_ec(static_cast<commonStub::ErrorCode>(ec));
        return grpc::Status::OK;
    }

    response->set_indication_count(curSessionStats_.indication_count);
    response->set_hostile_score_count(curSessionStats_.hostile_score_count);
    response->set_average_score(curSessionStats_.average_score);
    response->set_categories_detected(curSessionStats_.categories_detected);
    response->set_most_recent_policy_acted(curSessionStats_.most_recent_policy_acted);
    response->set_was_countermeasure_enacted(curSessionStats_.was_countermeasure_enacted);

    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

grpc::Status SecurityCCSServerImpl::registerCCSListener(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request,
    ::commonStub::ErrorCodeMsg* response) {

    std::string ecStr = "";
    telux::common::ErrorCode ec{};

    ecStr = apiConfigJsonRoot_["ICellularSecurityManager"]["registerListener"]["error"].asString();
    ec = CommonUtils::mapErrorCode(ecStr);
    response->set_ec(static_cast<commonStub::ErrorCode>(ec));

    return grpc::Status::OK;
}

grpc::Status SecurityCCSServerImpl::deRegisterCCSListener(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request,
    ::commonStub::ErrorCodeMsg* response) {

    std::string ecStr = "";
    telux::common::ErrorCode ec{};

    ecStr = apiConfigJsonRoot_["ICellularSecurityManager"]["deRegisterListener"][
        "error"].asString();
    ec = CommonUtils::mapErrorCode(ecStr);
    response->set_ec(static_cast<commonStub::ErrorCode>(ec));

    return grpc::Status::OK;
}


/*
 * Handle events injected externally by the user.
 */
void SecurityCCSServerImpl::onEventUpdate(::eventService::UnsolicitedEvent usrEvent) {
    LOG(DEBUG,__FUNCTION__);

    std::string event;
    std::string token;

    if (usrEvent.filter() != CCS_FILTER) {
        return;
    }

    event = usrEvent.event();

    token = EventParserUtil::getNextToken(event, CCS_DEFAULT_DELIMITER);
    if (token == "ssr") {
        handleSSREvent(event);
    } else if (token == "sec_report") {
        handleSecurityReportEvent(event);
    } else {
    }
}

/*
 * Receive report injected by the user. Translate it and dispatch for application.
 *
 * telsdk_event_injector -f ccs -e ssr SERVICE_UNAVAILABLE
 */
void SecurityCCSServerImpl::handleSSREvent(std::string eventParams) {

    ::securityStub::CCSServiceState newServiceState{};
    ::eventService::EventResponse anyResponse{};

    if (eventParams == "SERVICE_AVAILABLE") {
        newServiceState.set_value(::securityStub::CCSServiceState_Value_SSGCCS_ONLINE);
    } else if (eventParams == "SERVICE_UNAVAILABLE" || eventParams == "SERVICE_FAILED") {
        newServiceState.set_value(::securityStub::CCSServiceState_Value_SSGCCS_OFFLINE);
    } else {
        LOG(ERROR, __FUNCTION__, " invalid parameters: ", eventParams);
        return;
    }

    anyResponse.set_filter(CCS_FILTER);
    anyResponse.mutable_any()->PackFrom(newServiceState);
    clientEvent_.updateEventQueue(anyResponse);
}

/*
 * Receive report injected by the user. Translate it and dispatch for application.
 *
 * telsdk_event_injector -f ccs -e sec_report threat_score 250 cell_id 3 pid 1
 * mcc 311 mnc 030 threats 12 action_type 0 rat 4 env_state 2
 */
void SecurityCCSServerImpl::handleSecurityReportEvent(std::string eventParams) {

    int32_t score = 0;
    uint32_t cid = 0;
    uint32_t pid = 0;
    uint32_t mcc = 0;
    uint32_t mnc = 0;
    uint32_t rat = 0;
    uint32_t envState = 0;
    uint32_t teluxPolicy = 0;
    ssgccs_client_policy_action_t ssgPolicy = POLICY_NO_ACTION;
    uint32_t threatTypesBitMask = 0;
    uint32_t category = 0;

    std::string token;
    ::securityStub::CCSReport report{};
    ::eventService::EventResponse anyResponse{};

    while (1) {
        token = EventParserUtil::getNextToken(eventParams, CCS_DEFAULT_DELIMITER);
        if (token.empty()) {
            break;
        }
        if (token == "threat_score") {
            token = EventParserUtil::getNextToken(eventParams, CCS_DEFAULT_DELIMITER);
            try {
                score = std::stoi(token);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret score ", token);
                return;
            }
        } else if (token == "cell_id") {
            token = EventParserUtil::getNextToken(eventParams, CCS_DEFAULT_DELIMITER);
            try {
                cid = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret cid ", token);
                return ;
            }
        } else if (token == "pid") {
            token = EventParserUtil::getNextToken(eventParams, CCS_DEFAULT_DELIMITER);
            try {
                pid = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret pid ", token);
                return ;
            }
        } else if (token == "mcc") {
            token = EventParserUtil::getNextToken(eventParams, CCS_DEFAULT_DELIMITER);
            try {
                mcc = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret mcc ", token);
                return ;
            }
        } else if (token == "mnc") {
            token = EventParserUtil::getNextToken(eventParams, CCS_DEFAULT_DELIMITER);
            try {
                mnc = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret mnc ", token);
                return ;
            }
        } else if (token == "threats") {
            token = EventParserUtil::getNextToken(eventParams, CCS_DEFAULT_DELIMITER);
            try {
                threatTypesBitMask = std::stoul(token, nullptr, 10);
                toSSGCategory(threatTypesBitMask, category);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret threats ", token);
                return ;
            }
        } else if (token == "action_type") {
            token = EventParserUtil::getNextToken(eventParams, CCS_DEFAULT_DELIMITER);
            try {
                teluxPolicy = std::stoul(token, nullptr, 10);
                toSSGPolicy(teluxPolicy, ssgPolicy);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret policy ", token);
                return ;
            }
        } else if (token == "rat") {
            token = EventParserUtil::getNextToken(eventParams, CCS_DEFAULT_DELIMITER);
            try {
                rat = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret RAT ", token);
                return ;
            }
        } else if (token == "env_state") {
            token = EventParserUtil::getNextToken(eventParams, CCS_DEFAULT_DELIMITER);
            try {
                envState = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret env_state ", token);
                return ;
            }
        } else {
        }
    }

    /* Send report */
    report.set_score(score);
    report.set_cid(cid);
    report.set_pid(pid);
    report.set_mcc(mcc);
    report.set_mnc(mnc);
    report.set_category(category);
    report.set_policy_acted(static_cast<uint32_t>(ssgPolicy));
    report.set_env_state(static_cast<::securityStub::CCSReport_EnvState>(envState));
    report.set_radio(static_cast<::securityStub::CCSReport_RadioTech>(rat));

    anyResponse.set_filter(CCS_FILTER);
    anyResponse.mutable_any()->PackFrom(report);
    clientEvent_.updateEventQueue(anyResponse);

    /* Update internal stats */
    curSessionStats_.indication_count++;

    if (static_cast<uint32_t>(score) > SSGCCS_HOSTILE_SCORE) {
        curSessionStats_.hostile_score_count++;
    }

    curSessionStats_.average_score = (curSessionStats_.average_score + score) /
        curSessionStats_.indication_count;

    curSessionStats_.categories_detected |= category;

    curSessionStats_.most_recent_policy_acted = toPolicyStr(ssgPolicy);

    if (ssgPolicy != POLICY_NO_ACTION) {
        curSessionStats_.was_countermeasure_enacted = true;
    }
}

void SecurityCCSServerImpl::toSSGPolicy(uint32_t teluxPolicy,
    ssgccs_client_policy_action_t &ssgPolicy) {

    switch (static_cast<telux::sec::ActionType>(teluxPolicy)) {
        case telux::sec::ActionType::NONE:
            ssgPolicy = POLICY_NO_ACTION;
            break;
          case telux::sec::ActionType::DEPRIORITIZED:
            ssgPolicy = POLICY_DEPRIORITIZE;
            break;
        case telux::sec::ActionType::REMOVED_DEPRIORITIZATION:
            ssgPolicy = POLICY_UNDEPRIORITIZE;
            break;
        case telux::sec::ActionType::CELL_BARRED:
            ssgPolicy = POLICY_BAR;
            break;
        case telux::sec::ActionType::REMOVED_CELL_BARRING:
            ssgPolicy = POLICY_UNBAR;
            break;
        case telux::sec::ActionType::INVALID:
            ssgPolicy = POLICY_ABNORMAL;
            break;
        default:
            break;
    }
}

void SecurityCCSServerImpl::toSSGCategory(uint32_t threatTypesBitMask, uint32_t &category) {

    if ((threatTypesBitMask & static_cast<uint32_t>(telux::sec::CellularThreatType::UNKNOWN))
            == static_cast<uint32_t>(telux::sec::CellularThreatType::UNKNOWN)) {
        category |= CATEGORY_FLAG_UNKNOWN;
    }
    if ((threatTypesBitMask & static_cast<uint32_t>(telux::sec::CellularThreatType::IMPRISON))
            == static_cast<uint32_t>(telux::sec::CellularThreatType::IMPRISON)) {
        category |= CATEGORY_FLAG_IMPRISONER;
    }
    if ((threatTypesBitMask & static_cast<uint32_t>(telux::sec::CellularThreatType::DOS))
            == static_cast<uint32_t>(telux::sec::CellularThreatType::DOS)) {
        category |= CATEGORY_FLAG_DOS;
    }
    if ((threatTypesBitMask & static_cast<uint32_t>(telux::sec::CellularThreatType::DOWNGRADE))
            == static_cast<uint32_t>(telux::sec::CellularThreatType::DOWNGRADE)) {
        category |= CATEGORY_FLAG_DOWNGRADE;
    }
    if ((threatTypesBitMask & static_cast<uint32_t>(
            telux::sec::CellularThreatType::LOCATION_TRACKED_USING_IMSI))
            == static_cast<uint32_t>(
            telux::sec::CellularThreatType::LOCATION_TRACKED_USING_IMSI)) {
        category |= CATEGORY_FLAG_LOCATION_TRACKER;
    }
    if ((threatTypesBitMask & static_cast<uint32_t>(
            telux::sec::CellularThreatType::LOCATION_TRACKED_USING_AUTH))
            == static_cast<uint32_t>(
            telux::sec::CellularThreatType::LOCATION_TRACKED_USING_AUTH)) {
        category |= CATEGORY_FLAG_LOCATION_TRACKER_AUTH_REQEUST;
    }
    if ((threatTypesBitMask & static_cast<uint32_t>(telux::sec::CellularThreatType::PERSUADE))
            == static_cast<uint32_t>(telux::sec::CellularThreatType::PERSUADE)) {
        category |= CATEGORY_FLAG_ATTRACTIVE;
    }
    if ((threatTypesBitMask & static_cast<uint32_t>(
            telux::sec::CellularThreatType::NO_THREAT_DETECTED))
            == static_cast<uint32_t>(telux::sec::CellularThreatType::NO_THREAT_DETECTED)) {
        category |= CATEGORY_FLAG_DOWNGRADE;
    }
    if ((threatTypesBitMask & static_cast<uint32_t>(
            telux::sec::CellularThreatType::NO_ENCRYPTION))
            == static_cast<uint32_t>(telux::sec::CellularThreatType::NO_ENCRYPTION)) {
        category |= CATEGORY_FLAG_NO_ENCRYPTION;
    }
    if ((threatTypesBitMask & static_cast<uint32_t>(
            telux::sec::CellularThreatType::WEAK_ENCRYPTION))
            == static_cast<uint32_t>(telux::sec::CellularThreatType::WEAK_ENCRYPTION)) {
        category |= CATEGORY_FLAG_WEAK_ENCRYPTION;
    }
    if ((threatTypesBitMask & static_cast<uint32_t>(
            telux::sec::CellularThreatType::SELF_BLACKLISTING_CELL))
            == static_cast<uint32_t>(telux::sec::CellularThreatType::SELF_BLACKLISTING_CELL)) {
        category |= CATEGORY_FLAG_SELF_BLACKLISTING_CELL;
    }
    if ((threatTypesBitMask & static_cast<uint32_t>(
            telux::sec::CellularThreatType::UNAUTHENTICATED_SMS))
            == static_cast<uint32_t>(telux::sec::CellularThreatType::UNAUTHENTICATED_SMS)) {
        category |= CATEGORY_FLAG_UNAUTH_SMS;
    }
    if ((threatTypesBitMask & static_cast<uint32_t>(
            telux::sec::CellularThreatType::UNAUTHENTICATED_EMERGENCY_MESSAGE))
            == static_cast<uint32_t>(
            telux::sec::CellularThreatType::UNAUTHENTICATED_EMERGENCY_MESSAGE)) {
        category |= CATEGORY_FLAG_UNAUTH_EMERGENCY_MSG;
    }
    if ((threatTypesBitMask & static_cast<uint32_t>(
            telux::sec::CellularThreatType::IMSI_LEAK))
            == static_cast<uint32_t>(telux::sec::CellularThreatType::IMSI_LEAK)) {
        category |= CATEGORY_FLAG_IMSI_LEAK;
    }
}

std::string SecurityCCSServerImpl::toPolicyStr(ssgccs_client_policy_action_t ssgPolicy) {

    switch (ssgPolicy) {
        case POLICY_NO_ACTION:
            return "No Action";
          case POLICY_DEPRIORITIZE:
            return "Deprioritized";
        case POLICY_UNDEPRIORITIZE:
            return "Undeprioritized";
        case POLICY_BAR:
            return "Barred";
        case POLICY_UNBAR:
            return "Unbarred";
        case POLICY_ABNORMAL:
            return "Abnormal Action";
        default:
            return "Unknown";
    }
}
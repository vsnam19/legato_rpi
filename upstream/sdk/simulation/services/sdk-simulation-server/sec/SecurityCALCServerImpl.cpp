/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "libs/common/Logger.hpp"
#include "libs/common/CommonUtils.hpp"

#include "SecurityCALCServerImpl.hpp"

SecurityCALCServerImpl::SecurityCALCServerImpl()
    : serverEvent_(ServerEventManager::getInstance())
    , clientEvent_(EventService::getInstance()) {

    curCapacity_.sm2 = COMMON_MAX_CAPACITY;
    curCapacity_.nist256 = COMMON_MAX_CAPACITY;
    curCapacity_.nist384 = NISTP384_MAX_CAPACITY;
    curCapacity_.bp256 = COMMON_MAX_CAPACITY;
    curCapacity_.bp384 = BP384_MAX_CAPACITY;
}

SecurityCALCServerImpl::~SecurityCALCServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status SecurityCALCServerImpl::Init(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request, ::commonStub::ErrorCodeMsg* response) {

    telux::common::ErrorCode ec;
    telux::common::Status status;

    if (isServiceInitialized_) {
        clientsCount_++;
        response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
        return grpc::Status::OK;
    }

    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, CALC_API_JSON_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", CALC_API_JSON_FILE);
            response->set_ec(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    status = serverEvent_.registerListener(shared_from_this(), CALC_FILTER);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't register with ServerEventManager");
        response->set_ec(commonStub::ErrorCode::SYSTEM_ERR);
        return grpc::Status::OK;
    }

    randRing_.seed(12);
    randUniDistributionCommon_ = std::uniform_int_distribution<uint32_t>(0, COMMON_MAX_CAPACITY);
    randUniDistributionNist384_ = std::uniform_int_distribution<uint32_t>(0, NISTP384_MAX_CAPACITY);
    randUniDistributionBp384_ = std::uniform_int_distribution<uint32_t>(0, BP384_MAX_CAPACITY);

    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    curLoadCount_ = {};
    injectedLoadCount_ = {};

    clientsCount_++;
    isServiceInitialized_ = true;
    return grpc::Status::OK;
}

grpc::Status SecurityCALCServerImpl::DeInit(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request, ::commonStub::ErrorCodeMsg* response) {

    clientsCount_--;
    if (!clientsCount_) {
        serverEvent_.deregisterListener(shared_from_this(), CALC_FILTER);
        isServiceInitialized_ = false;
    }

    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

grpc::Status SecurityCALCServerImpl::GetCapacity(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request, ::securityStub::Capacity* response) {

    std::string ecStr = "";
    telux::common::ErrorCode ec{};

    ecStr = apiConfigJsonRoot_["ICAControlManager"]["getCapacity"]["error"].asString();
    ec = CommonUtils::mapErrorCode(ecStr);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        response->set_error_code(static_cast<commonStub::ErrorCode>(ec));
        return grpc::Status::OK;
    }

    response->set_sm2(curCapacity_.sm2);
    response->set_nist256(curCapacity_.nist256);
    response->set_nist384(curCapacity_.nist384);
    response->set_bp256(curCapacity_.bp256);
    response->set_bp384(curCapacity_.bp384);
    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

/*
 * Equivalent to mvm_stats_get_msg_count().
 */
grpc::Status SecurityCALCServerImpl::GetOperationsCount(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request, ::securityStub::LoadCount* response) {

    if (haveInjectedLoad_) {
        /* provide user given load */
        curLoadCount_.sm2 = curLoadCount_.sm2 + injectedLoadCount_.sm2;
        response->set_sm2(curLoadCount_.sm2);
        curLoadCount_.nist256 = curLoadCount_.nist256 + injectedLoadCount_.nist256;
        response->set_nist256(curLoadCount_.nist256);
        curLoadCount_.nist384 = curLoadCount_.nist384 + injectedLoadCount_.nist384;
        response->set_nist384(curLoadCount_.nist384);
        curLoadCount_.bp256 = curLoadCount_.bp256 + injectedLoadCount_.bp256;
        response->set_bp256(curLoadCount_.bp256);
        curLoadCount_.bp384 = curLoadCount_.bp384 + injectedLoadCount_.bp384;
        response->set_bp384(curLoadCount_.bp384);
        response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
        haveInjectedLoad_ = false;
        injectedLoadCount_ = {};
        return grpc::Status::OK;
    }

    /* provide internally calculated load */
    curLoadCount_.sm2 += randUniDistributionCommon_(randRing_);
    curLoadCount_.nist256 += randUniDistributionCommon_(randRing_);
    curLoadCount_.nist384 += randUniDistributionNist384_(randRing_);
    curLoadCount_.bp256 += randUniDistributionCommon_(randRing_);
    curLoadCount_.bp384 += randUniDistributionBp384_(randRing_);

    response->set_sm2(curLoadCount_.sm2);
    response->set_nist256(curLoadCount_.nist256);
    response->set_nist384(curLoadCount_.nist384);
    response->set_bp256(curLoadCount_.bp256);
    response->set_bp384(curLoadCount_.bp384);
    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

grpc::Status SecurityCALCServerImpl::RegisterClient(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request,
    ::commonStub::ErrorCodeMsg* response) {

    std::string ecStr = "";
    telux::common::ErrorCode ec;
    ecStr = apiConfigJsonRoot_["ICAControlManager"]["registerListener"]["error"].asString();
    ec = CommonUtils::mapErrorCode(ecStr);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        response->set_ec(static_cast<commonStub::ErrorCode>(ec));
        return grpc::Status::OK;
    }

    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    return grpc::Status::OK;
}

grpc::Status SecurityCALCServerImpl::DeregisterClient(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request,
    ::commonStub::ErrorCodeMsg* response) {
    std::string ecStr = "";
    telux::common::ErrorCode ec;

    ecStr = apiConfigJsonRoot_["ICAControlManager"]["deregisterListener"]["error"].asString();
    ec = CommonUtils::mapErrorCode(ecStr);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        response->set_ec(static_cast<commonStub::ErrorCode>(ec));
        return grpc::Status::OK;
    }

    response->set_ec(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    return grpc::Status::OK;
}

/*
 * Handle load events injected externally by the user.
 *
 * Capacity never changes, therefore callback registered with libmvm
 * is never called even on target, hence not implemented here.
 */
void SecurityCALCServerImpl::onEventUpdate(::eventService::UnsolicitedEvent usrEvent) {
    LOG(DEBUG,__FUNCTION__);

    std::string event;
    std::string token;

    if (usrEvent.filter() != CALC_FILTER) {
        return;
    }

    event = usrEvent.event();
    token = EventParserUtil::getNextToken(event, CALC_DEFAULT_DELIMITER);

    if (token == "load") {
        handleLoadEvent(event);
    } else if (token == "capacity") {
        handleCapacityEvent(event);
    } else {
    }
}

/*
 * telsdk_event_injector -f calc -e load sm2 12 nist256 5 nist384 9 bp256 7 bp384 21
 */
void SecurityCALCServerImpl::handleLoadEvent(std::string eventParams) {

    std::string token;

    while (1) {
        token = EventParserUtil::getNextToken(eventParams, CALC_DEFAULT_DELIMITER);
        if (token.empty()) {
            break;
        }
        if (token == "sm2") {
            token = EventParserUtil::getNextToken(eventParams, CALC_DEFAULT_DELIMITER);
            try {
                injectedLoadCount_.sm2 = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret sm2 value ", token);
                return ;
            }
        } else if (token == "nist256") {
            token = EventParserUtil::getNextToken(eventParams, CALC_DEFAULT_DELIMITER);
            try {
                injectedLoadCount_.nist256 = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret nist256 value ", token);
                return ;
            }
        } else if (token == "nist384") {
            token = EventParserUtil::getNextToken(eventParams, CALC_DEFAULT_DELIMITER);
            try {
                injectedLoadCount_.nist384 = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret nist384 value ", token);
                return ;
            }
        } else if (token == "bp256") {
            token = EventParserUtil::getNextToken(eventParams, CALC_DEFAULT_DELIMITER);
            try {
                injectedLoadCount_.bp256 = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret bp256 value ", token);
                return ;
            }
        } else if (token == "bp384") {
            token = EventParserUtil::getNextToken(eventParams, CALC_DEFAULT_DELIMITER);
            try {
                injectedLoadCount_.bp384 = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret bp384 value ", token);
                return ;
            }
        } else {
        }
    }

    haveInjectedLoad_ = true;
}

/*
 * telsdk_event_injector -f calc -e capacity sm2 4000 nist256 4000 nist384 2200 bp256 4000 bp384
 * 1500
 */
void SecurityCALCServerImpl::handleCapacityEvent(std::string eventParams) {

    std::string token;
    ::securityStub::Capacity capacity{};
    ::eventService::EventResponse anyResponse{};

    while (1) {
        token = EventParserUtil::getNextToken(eventParams, CALC_DEFAULT_DELIMITER);
        if (token.empty()) {
            break;
        }
        if (token == "sm2") {
            token = EventParserUtil::getNextToken(eventParams, CALC_DEFAULT_DELIMITER);
            try {
                curCapacity_.sm2 = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret sm2 value ", token);
                return ;
            }
        } else if (token == "nist256") {
            token = EventParserUtil::getNextToken(eventParams, CALC_DEFAULT_DELIMITER);
            try {
                curCapacity_.nist256 = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret nist256 value ", token);
                return ;
            }
        } else if (token == "nist384") {
            token = EventParserUtil::getNextToken(eventParams, CALC_DEFAULT_DELIMITER);
            try {
                curCapacity_.nist384 = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret nist384 value ", token);
                return ;
            }
        } else if (token == "bp256") {
            token = EventParserUtil::getNextToken(eventParams, CALC_DEFAULT_DELIMITER);
            try {
                curCapacity_.bp256 = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret bp256 value ", token);
                return ;
            }
        } else if (token == "bp384") {
            token = EventParserUtil::getNextToken(eventParams, CALC_DEFAULT_DELIMITER);
            try {
                curCapacity_.bp384 = std::stoul(token, nullptr, 10);
            } catch (const std::exception &e) {
                LOG(ERROR, __FUNCTION__, " can't interpret bp384 value ", token);
                return ;
            }
        } else {
        }
    }

    capacity.set_sm2(curCapacity_.sm2);
    capacity.set_nist256(curCapacity_.nist256);
    capacity.set_nist384(curCapacity_.nist384);
    capacity.set_bp256(curCapacity_.bp256);
    capacity.set_bp384(curCapacity_.bp384);
    capacity.set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    anyResponse.set_filter(CALC_FILTER);
    anyResponse.mutable_any()->PackFrom(capacity);
    clientEvent_.updateEventQueue(anyResponse);
}
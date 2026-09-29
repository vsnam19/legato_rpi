/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "CellularSecurityManagerImpl.hpp"

namespace telux {
namespace sec {

CellularSecurityManagerImpl::CellularSecurityManagerImpl()
    : clientEventMgr_(ClientEventManager::getInstance()) {
    exitNow_ = false;
}

CellularSecurityManagerImpl::~CellularSecurityManagerImpl() {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lock(CellularSecurityManagerImpl::operationGuard_);

    exitNow_ = true;

    if (listenerExist_) {
        /* As the listener exist, these resources exist. Clean up them. */
        disconnectCCS(true);
    }
    stub_ = nullptr;
}

telux::common::ErrorCode CellularSecurityManagerImpl::init() {
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::CCSConnectInfo request{};
    commonStub::ErrorCodeMsg response{};

    telux::common::ErrorCode ec;

    stub_ = CommonUtils::getGrpcStub<securityStub::SecurityCCSService>();

    request.set_trailing_indication_queue_size(LAST_INDICATION_QUEUE_SIZE);

    reqStatus = stub_->Init(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " cant't init");
        return ec;
    }

    serviceStatus_ = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Disconnects with CCS framework to receive reports and ssr events.
 */
telux::common::ErrorCode CellularSecurityManagerImpl::disconnectCCS(bool isExiting_) {

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::CCSDisconnectInfo request{};

    telux::common::Status status;
    telux::common::ErrorCode ec{};
    commonStub::ErrorCodeMsg response{};

    if (!isExiting_) {
        status = clientEventMgr_.deregisterListener(shared_from_this(), CCS_FILTER);
        if ((status != telux::common::Status::SUCCESS) &&
            (status != telux::common::Status::ALREADY)) {
            LOG(ERROR, __FUNCTION__, " can't deregister with ClientEventManager");
            /* don't treat fatal */
        }
        return telux::common::ErrorCode::SUCCESS;
    }

    reqStatus = stub_->DeInit(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " cant't deinit");
        return ec;
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Connects with CCS framework to receive reports and ssr events.
 */
telux::common::ErrorCode CellularSecurityManagerImpl::reconnectCCS() {
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    commonStub::ErrorCodeMsg response{};

    telux::common::ErrorCode ec;
    telux::common::Status status;

    status = clientEventMgr_.registerListener(shared_from_this(), CCS_FILTER);
    if ((status != telux::common::Status::SUCCESS) &&
        (status != telux::common::Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, " can't register with ClientEventManager");
        return telux::common::CommonUtils::toErrorCode(status);
    }

    reqStatus = stub_->registerCCSListener(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't register listener, err ", static_cast<int>(ec));
        return ec;
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Allocates resources and initializes them as applicable.
 * Registers listener to receive reports. Register only once with the
 * ccs framework. For subsequent registrations, new listener is just
 * appended to the internal list.
 */
telux::common::ErrorCode CellularSecurityManagerImpl::registerListener(
    std::weak_ptr<ICellularScanReportListener> listener) {

    std::string ecStr = "";
    telux::common::ErrorCode ec;
    telux::common::Status status;
    std::vector<std::weak_ptr<telux::common::ISDKListener>> listenerList;

    if (serviceStatus_ == ServiceStatus::SERVICE_UNAVAILABLE ||
        serviceStatus_ == ServiceStatus::SERVICE_FAILED) {
        LOG(ERROR, __FUNCTION__, " Service is unavailable or failed");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    {
        std::lock_guard<std::mutex> lock(CellularSecurityManagerImpl::operationGuard_);

        if (listenerExist_) {
            /* at-least 1 listener already registered, therefore,
             * just add this new listener to the list. */
            status = csListenerMgr_->registerListener(listener);
            if (status != telux::common::Status::SUCCESS) {
                return telux::common::CommonUtils::toErrorCode(status);
            }

            return telux::common::ErrorCode::SUCCESS;
        }

        try {
            csListenerMgr_ = std::make_shared<
                telux::common::ListenerManager<ICellularScanReportListener>>();
        } catch (const std::exception &e) {
            LOG(ERROR, __FUNCTION__, " can't create listeners manager");
            return telux::common::ErrorCode::NO_MEMORY;
        }

        try {
            reportAndSSRDispatcher_ = std::make_shared<telux::common::TaskDispatcher>();
        } catch (const std::exception &e) {
            LOG(ERROR, __FUNCTION__, " can't create TaskDispatcher");
            ec = telux::common::ErrorCode::NO_MEMORY;
            goto err1;
        }

        status = csListenerMgr_->registerListener(listener);
        if (status != telux::common::Status::SUCCESS) {
            ec = telux::common::CommonUtils::toErrorCode(status);
            goto err2;
        }

        ec = reconnectCCS();
        if (ec != telux::common::ErrorCode::SUCCESS) {
            goto err2;
        }

        listenerExist_ = true;
        return telux::common::ErrorCode::SUCCESS;

    err2:
        reportAndSSRDispatcher_ = nullptr;
    err1:
        csListenerMgr_ = nullptr;

        return ec;
    }
}

/*
 * Deregisters given listener previously registered with registerListener().
 */
telux::common::ErrorCode CellularSecurityManagerImpl::deRegisterListener(
    std::weak_ptr<ICellularScanReportListener> listener) {

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    commonStub::ErrorCodeMsg response{};

    telux::common::ErrorCode ec;
    telux::common::Status status;
    std::vector<std::weak_ptr<ICellularScanReportListener>> listenerList;

    if (serviceStatus_ == ServiceStatus::SERVICE_UNAVAILABLE ||
        serviceStatus_ == ServiceStatus::SERVICE_FAILED) {
        LOG(ERROR, __FUNCTION__, " Service is unavailable or failed");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    reqStatus = stub_->deRegisterCCSListener(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't deRegisterListener, err ", static_cast<int>(ec));
        return ec;
    }

    {
        std::lock_guard<std::mutex> lock(CellularSecurityManagerImpl::operationGuard_);

        status = csListenerMgr_->deRegisterListener(listener);
        if (status != telux::common::Status::SUCCESS) {
            return telux::common::CommonUtils::toErrorCode(status);
        }

        csListenerMgr_->getAvailableListeners(listenerList);
        if (!listenerList.size()) {
            /* If this is the last listener, deregister with lower layers as well
             * to avoid unnecessary updates as there is no application listener. */
            ec = disconnectCCS(false);
            if (ec != telux::common::ErrorCode::SUCCESS) {
                LOG(ERROR, __FUNCTION__, " can't deregister, err ", static_cast<int>(ec));
                /* don't treate this as fatal, continue further */
            }

            /* clean up resources as there is no listener */
            reportAndSSRDispatcher_ = nullptr;
            csListenerMgr_          = nullptr;
            listenerExist_ = false;
        }

        return telux::common::ErrorCode::SUCCESS;
    }
}

telux::common::ErrorCode CellularSecurityManagerImpl::getCurrentSessionStats(
    SessionStats &sessionStats) {

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};

    telux::common::ErrorCode ec{};
    ::securityStub::SessionStats response{};

    if (serviceStatus_ == ServiceStatus::SERVICE_UNAVAILABLE ||
        serviceStatus_ == ServiceStatus::SERVICE_FAILED) {
        LOG(ERROR, __FUNCTION__, " Service is unavailable or failed");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    {
        reqStatus = stub_->GetCurrentSessionStats(&clientCtx, request, &response);
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " communication error");
            return telux::common::ErrorCode::TRANSPORT_ERROR;
        }

        ec = static_cast<telux::common::ErrorCode>(response.ec());
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't get stats, err ", static_cast<int>(ec));
            return ec;
        }

        sessionStats.reportsCount = response.indication_count();
        sessionStats.thresholdCrossedCount = response.hostile_score_count();
        sessionStats.averageThreatScore = response.average_score();
        sessionStats.anyActionTaken = static_cast<bool>(response.was_countermeasure_enacted());

        if (!(response.most_recent_policy_acted().compare("No Action"))) {
            sessionStats.lastAction = ActionType::NONE;
        } else if (!(response.most_recent_policy_acted().compare("Deprioritized"))) {
            sessionStats.lastAction = ActionType::DEPRIORITIZED;
        } else if (!(response.most_recent_policy_acted().compare("Barred"))) {
            sessionStats.lastAction = ActionType::CELL_BARRED;
        } else if (!(response.most_recent_policy_acted().compare("Abnormal Action"))) {
            sessionStats.lastAction = ActionType::INVALID;
        } else if (!(response.most_recent_policy_acted().compare("Unknown"))) {
            sessionStats.lastAction = ActionType::INVALID;
        } else if (!(response.most_recent_policy_acted().compare("Unbarred"))) {
            sessionStats.lastAction = ActionType::REMOVED_CELL_BARRING;
        } else if (!(response.most_recent_policy_acted().compare("Undeprioritized"))) {
            sessionStats.lastAction = ActionType::REMOVED_DEPRIORITIZATION;
        } else {
        }

        populateThreatTypes(response.categories_detected(), sessionStats.threats);

        return telux::common::ErrorCode::SUCCESS;
    }
}

int32_t CellularSecurityManagerImpl::rawReportHandler(::securityStub::CCSReport ccsReport,
    void *cookie) {

    EnvironmentInfo environmentInfo{};
    CellularSecurityReport finalReport{};

    finalReport.threatScore = ccsReport.score();
    finalReport.cellId = ccsReport.cid();
    finalReport.pid = ccsReport.pid();
    finalReport.mcc = std::to_string(ccsReport.mcc());
    finalReport.mnc = std::to_string(ccsReport.mnc());

    populateThreatTypes(ccsReport.category(), finalReport.threats);
    populatePolicy(ccsReport.policy_acted(), finalReport.actionType);
    populateRAT(static_cast<ssgccs_radio_enum_t>(ccsReport.radio()), finalReport.rat);
    populateEnvironmentState(static_cast<ssgccs_environmental_state_t>(ccsReport.env_state()),
        environmentInfo.environmentState);

    deliverReportOrSSREvent(true, telux::common::ServiceStatus::SERVICE_AVAILABLE, finalReport,
        environmentInfo, cookie);

    return 0;
}

void CellularSecurityManagerImpl::ssrHandler(ssgccs_state_t ccsState, void *cookie) {

    switch (ccsState) {
        case SSGCCS_OFFLINE:
            serviceStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
            break;
        case SSGCCS_ONLINE:
            serviceStatus_ = telux::common::ServiceStatus::SERVICE_AVAILABLE;
            break;
        default:
            /* just emit log for debugging */
            LOG(DEBUG, __FUNCTION__, " dropping new state ", static_cast<int>(ccsState));
            return;
    };

    deliverReportOrSSREvent(
        false, serviceStatus_, CellularSecurityReport(), EnvironmentInfo(), cookie);
}

void CellularSecurityManagerImpl::deliverReportOrSSREvent(bool isReport,
    telux::common::ServiceStatus serviceStatus, CellularSecurityReport finalReport,
    EnvironmentInfo environmentInfo, void *cookie) {

    std::shared_ptr<CellularSecurityManagerImpl> csMgrImpl;
    std::vector<std::weak_ptr<ICellularScanReportListener>> listenerList;

    {
        std::lock_guard<std::mutex> lock(CellularSecurityManagerImpl::operationGuard_);

        if (exitNow_) {
            /* CellularSecurityManagerImpl has been destructed */
            LOG(DEBUG, __FUNCTION__, " CellularSecurityManagerImpl destructed");
            return;
        }

        csMgrImpl = shared_from_this();

        csMgrImpl->csListenerMgr_->getAvailableListeners(listenerList);
        if (!listenerList.size()) {
            LOG(ERROR, __FUNCTION__, " no listener");
            return;
        }

        for (size_t x = 0; x < listenerList.size(); x++) {
            if (auto sp
                = std::dynamic_pointer_cast<ICellularScanReportListener>(listenerList[x].lock())) {
                csMgrImpl->reportAndSSRDispatcher_->submitTask([=] {
                    if (isReport) {
                        sp->onScanReportAvailable(finalReport, environmentInfo);
                    } else {
                        sp->onServiceStatusChange(serviceStatus);
                    }
                });
                continue;
            }

            LOG(ERROR, __FUNCTION__, " no listener");
            /* don't treat as error, application just released listener */
        }

        if (!isReport && (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)) {
            /*
             * Init CCS framework again. This is done from the dispatcher thread to
             * ensure correct ordering. Until all SSR handling is not compelete, no
             * report will be sent to the application.
             */
            csMgrImpl->reportAndSSRDispatcher_->submitTask([=] { csMgrImpl->reconnectCCS(); });
        }
    }
}

void CellularSecurityManagerImpl::populateRAT(ssgccs_radio_enum_t radio, RATType &ratType) {

    switch (radio) {
        case RADIO_GERAN:
            ratType = RATType::GSM;
            break;
        case RADIO_WCDMA:
            ratType = RATType::WCDMA;
            break;
        case RADIO_LTE:
            ratType = RATType::LTE;
            break;
        case RADIO_NR:
            ratType = RATType::NR5G;
            break;
        default:
            ratType = RATType::UNKNOWN;
            break;
    }
}

void CellularSecurityManagerImpl::populatePolicy(
    uint32_t policyActed, ActionType &actionType) {

    switch (policyActed) {
        case POLICY_NO_ACTION:
            actionType = ActionType::NONE;
            break;
        case POLICY_DEPRIORITIZE:
            actionType = ActionType::DEPRIORITIZED;
            break;
        case POLICY_UNDEPRIORITIZE:
            actionType = ActionType::REMOVED_DEPRIORITIZATION;
            break;
        case POLICY_BAR:
            actionType = ActionType::CELL_BARRED;
            break;
        case POLICY_UNBAR:
            actionType = ActionType::REMOVED_CELL_BARRING;
            break;
        case POLICY_ABNORMAL:
            actionType = ActionType::INVALID;
            break;
        default:
            break;
    }
}

void CellularSecurityManagerImpl::populateEnvironmentState(
    ssgccs_environmental_state_t envState, EnvironmentState &environmentState) {

    switch (envState) {
        case SAFE:
            environmentState = EnvironmentState::SAFE;
            break;
        case ALERT:
            environmentState = EnvironmentState::ALERT;
            break;
        case HOSTILE:
            environmentState = EnvironmentState::HOSTILE;
            break;
        case CCS_UNKNOWN:
        default:
            environmentState = EnvironmentState::UNKNOWN;
            break;
    }
}

void CellularSecurityManagerImpl::populateThreatTypes(
    uint32_t category, std::vector<CellularThreatType> &threats) {

    if (category == 0) {
        threats.emplace_back(CellularThreatType::UNKNOWN);
    }

    if ((category & CATEGORY_FLAG_IMSI_LEAK) == CATEGORY_FLAG_IMSI_LEAK) {
        threats.emplace_back(CellularThreatType::IMSI_LEAK);
    }

    if ((category & CATEGORY_FLAG_IMPRISONER) == CATEGORY_FLAG_IMPRISONER) {
        threats.emplace_back(CellularThreatType::IMPRISON);
    }

    if ((category & CATEGORY_FLAG_DOS) == CATEGORY_FLAG_DOS) {
        threats.emplace_back(CellularThreatType::DOS);
    }

    if ((category & CATEGORY_FLAG_DOWNGRADE) == CATEGORY_FLAG_DOWNGRADE) {
        threats.emplace_back(CellularThreatType::DOWNGRADE);
    }

    if ((category & CATEGORY_FLAG_LOCATION_TRACKER) == CATEGORY_FLAG_LOCATION_TRACKER) {
        threats.emplace_back(CellularThreatType::LOCATION_TRACKED_USING_IMSI);
    }

    if ((category & CATEGORY_FLAG_ATTRACTIVE) == CATEGORY_FLAG_ATTRACTIVE) {
        threats.emplace_back(CellularThreatType::PERSUADE);
    }

    if ((category & CATEGORY_FLAG_AUTH_PASSED) == CATEGORY_FLAG_AUTH_PASSED) {
        threats.emplace_back(CellularThreatType::NO_THREAT_DETECTED);
    }

    if ((category & CATEGORY_FLAG_NO_ENCRYPTION) == CATEGORY_FLAG_NO_ENCRYPTION) {
        threats.emplace_back(CellularThreatType::NO_ENCRYPTION);
    }

    if ((category & CATEGORY_FLAG_WEAK_ENCRYPTION) == CATEGORY_FLAG_WEAK_ENCRYPTION) {
        threats.emplace_back(CellularThreatType::WEAK_ENCRYPTION);
    }

    if ((category & CATEGORY_FLAG_SELF_BLACKLISTING_CELL) == CATEGORY_FLAG_SELF_BLACKLISTING_CELL) {
        threats.emplace_back(CellularThreatType::SELF_BLACKLISTING_CELL);
    }

    if ((category & CATEGORY_FLAG_UNAUTH_SMS) == CATEGORY_FLAG_UNAUTH_SMS) {
        threats.emplace_back(CellularThreatType::UNAUTHENTICATED_SMS);
    }

    if ((category & CATEGORY_FLAG_UNAUTH_EMERGENCY_MSG) == CATEGORY_FLAG_UNAUTH_EMERGENCY_MSG) {
        threats.emplace_back(CellularThreatType::UNAUTHENTICATED_EMERGENCY_MESSAGE);
    }

    if ((category & CATEGORY_FLAG_LOCATION_TRACKER_AUTH_REQEUST)
        == CATEGORY_FLAG_LOCATION_TRACKER_AUTH_REQEUST) {
        threats.emplace_back(CellularThreatType::LOCATION_TRACKED_USING_AUTH);
    }
}

/*
 * Receives events via simulation event manager framework.
 */
void CellularSecurityManagerImpl::onEventUpdate(google::protobuf::Any event) {

    ::securityStub::CCSReport ccsReport;
    ::securityStub::CCSServiceState newServiceState;

    if (event.Is<::securityStub::CCSReport>()) {
        event.UnpackTo(&ccsReport);
        rawReportHandler(ccsReport, (void *)NULL);
    } else if (event.Is<::securityStub::CCSServiceState>()) {
        event.UnpackTo(&newServiceState);
        ssrHandler(static_cast<ssgccs_state_t>(newServiceState.value()), (void *)NULL);
    } else {
    }
}

}  // End of namespace sec
}  // End of namespace telux

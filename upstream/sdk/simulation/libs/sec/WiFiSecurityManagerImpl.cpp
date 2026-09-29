/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <chrono>

#include "WiFiSecurityManagerImpl.hpp"

namespace telux {
namespace sec {

std::mutex WiFiSecurityManagerImpl::operationGuard_;

WiFiSecurityManagerImpl::WiFiSecurityManagerImpl()
   : initCb_(nullptr)
   , serviceStatusListenerMgr_(nullptr)
   , clientEventMgr_(ClientEventManager::getInstance()) {
}

WiFiSecurityManagerImpl::~WiFiSecurityManagerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

void WiFiSecurityManagerImpl::cleanup() {
}

ServiceStatus WiFiSecurityManagerImpl::convertServiceStatus() {

    if (serviceStatus_ == ServiceStatus::SERVICE_UNAVAILABLE) {
        return ServiceStatus::SERVICE_UNAVAILABLE;
    } else if (serviceStatus_ == ServiceStatus::SERVICE_AVAILABLE) {
        return ServiceStatus::SERVICE_AVAILABLE;
    }

    return ServiceStatus::SERVICE_FAILED;
}

ServiceStatus WiFiSecurityManagerImpl::getServiceStatus() {
    return convertServiceStatus();
}

void WiFiSecurityManagerImpl::setServiceStatus(ServiceStatus status) {

    {
        std::lock_guard<std::mutex> lock(mutex_);
        serviceStatus_ = status;
        if (status != ServiceStatus::SERVICE_AVAILABLE) {
            isInitsyncTriggered_  = false;
        }

        if (initCb_) {
            initCb_(convertServiceStatus());
        }
    }

    // Send service status change across registered listeners.
    std::vector<std::weak_ptr<telux::common::IServiceStatusListener>> applisteners;
    serviceStatusListenerMgr_->getAvailableListeners(applisteners);
    if (applisteners.size() == 0) {
        return;
    }

    for (auto &wp : applisteners) {
        auto sp = wp.lock();
        if (sp) {
            sp->onServiceStatusChange(status);
        }
    }
}

/*
 * Setup/initiate connection to security QMI server.
 * Complete non-blocking initializations and schedule blocking ones.
 */
telux::common::Status WiFiSecurityManagerImpl::init(
    telux::common::InitResponseCb initResultListener) {

    telux::common::ErrorCode ec;
    telux::common::Status status;
    std::shared_future<void> future;

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    ::securityStub::InitInfo response{};

    try {
        serviceStatusListenerMgr_ = std::make_shared<
            telux::common::ListenerManager<IServiceStatusListener>>();
    } catch (const std::exception &e) {
        LOG(ERROR, __FUNCTION__, " can't setup ListenerManager");
        return telux::common::Status::FAILED;
    }

    status = clientEventMgr_.registerListener(shared_from_this(), WCS_FILTER);
    if ((status != telux::common::Status::SUCCESS) &&
        (status != telux::common::Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, " can't register with ClientEventManager");
        return status;
    }

    stub_ = CommonUtils::getGrpcStub<securityStub::SecurityWCSService>();

    reqStatus = stub_->Init(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::Status::FAILED;
    }

    ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " cant't init");
        return telux::common::Status::FAILED;
    }

    ss_ready_delay_ = response.ss_ready_delay();
    ss_service_status_ = static_cast<telux::common::ServiceStatus>(response.service_status());
    initCb_ = initResultListener;

    /* Schedule blocking initializations */
    future = std::async(std::launch::async, [this]() { this->initSync(); }).share();

    /* Hold onto future's reference until initSync() finishes */
    status = asyncTaskQueue_.add(future);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't add to queue");
        return status;
    }

    return telux::common::Status::SUCCESS;
}

void WiFiSecurityManagerImpl::initSync() {

    ServiceStatus serviceStatus = ServiceStatus::SERVICE_FAILED;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (isInitsyncTriggered_) {
            LOG(DEBUG, __FUNCTION__, " initialization is already triggered");
            return;
        }

        isInitsyncTriggered_ = true;
    }

    serviceStatus = ss_service_status_;
    std::this_thread::sleep_for(std::chrono::milliseconds(ss_ready_delay_));
    setServiceStatus(serviceStatus);
}

/*
 * Application registration for service status events.
 */
ErrorCode WiFiSecurityManagerImpl::registerListener(
    std::weak_ptr<IServiceStatusListener> listener) {
    telux::common::Status status;

    if (getServiceStatus() == ServiceStatus::SERVICE_UNAVAILABLE ||
        getServiceStatus() == ServiceStatus::SERVICE_FAILED) {
        LOG(ERROR, __FUNCTION__, " Service is unavailable or failed");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    status = serviceStatusListenerMgr_->registerListener(listener);

    return telux::common::CommonUtils::toErrorCode(status);
}

/*
 * Application de-registration for service status events.
 */
ErrorCode WiFiSecurityManagerImpl::deregisterListener(
    std::weak_ptr<IServiceStatusListener> listener) {
    telux::common::Status status;

    if (getServiceStatus() == ServiceStatus::SERVICE_UNAVAILABLE ||
        getServiceStatus() == ServiceStatus::SERVICE_FAILED) {
        LOG(ERROR, __FUNCTION__, " Service is unavailable or failed");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    status = serviceStatusListenerMgr_->deRegisterListener(listener);

    return telux::common::CommonUtils::toErrorCode(status);
}

/*
 * Allocates resources and initializes them as applicable.
 * Registers listener to receive reports. Register only once with the
 * wcs framework. For subsequent registrations, new listener is just
 * appended to the internal list.
 */
telux::common::ErrorCode WiFiSecurityManagerImpl::registerListener(
    std::weak_ptr<IWiFiReportListener> listener) {

    telux::common::ErrorCode ec;
    telux::common::Status status;

    if (getServiceStatus() == ServiceStatus::SERVICE_UNAVAILABLE ||
        getServiceStatus() == ServiceStatus::SERVICE_FAILED) {
        LOG(ERROR, __FUNCTION__, " Service is unavailable or failed");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    {
        std::lock_guard<std::mutex> lock(WiFiSecurityManagerImpl::operationGuard_);

        if (listenerExist_) {
            /* at-least 1 listener already registered, therefore,
             * just add this new listener to the list. */
            status = secReportListenerMgr_->registerListener(listener);
            if (status != telux::common::Status::SUCCESS) {
                return telux::common::CommonUtils::toErrorCode(status);
            }

            return telux::common::ErrorCode::SUCCESS;
        }

        try {
            secReportListenerMgr_ = std::make_shared<
                telux::common::ListenerManager<IWiFiReportListener>>();
        } catch (const std::exception &e) {
            LOG(ERROR, __FUNCTION__, " can't create listeners manager");
            return telux::common::CommonUtils::toErrorCode(telux::common::Status::FAILED);
        }

        status = secReportListenerMgr_->registerListener(listener);
        if (status != telux::common::Status::SUCCESS) {
            ec = telux::common::CommonUtils::toErrorCode(status);
            goto err;
        }

        ec = registerForSecReports();
        if (ec != telux::common::ErrorCode::SUCCESS) {
            goto err;
        }
        listenerExist_ = true;

        return telux::common::ErrorCode::SUCCESS;

    err:
        secReportListenerMgr_ = nullptr;
        return ec;
    }
}

/*
 * Deregisters given listener previously registered with registerSecReportListener().
 */
telux::common::ErrorCode WiFiSecurityManagerImpl::deregisterListener(
    std::weak_ptr<IWiFiReportListener> listener) {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    std::vector<std::weak_ptr<IWiFiReportListener>> listenerList;

    if (getServiceStatus() == ServiceStatus::SERVICE_UNAVAILABLE ||
        getServiceStatus() == ServiceStatus::SERVICE_FAILED) {
        LOG(ERROR, __FUNCTION__, " Service is unavailable or failed");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    {
        std::lock_guard<std::mutex> lock(WiFiSecurityManagerImpl::operationGuard_);

        if (!secReportListenerMgr_) {
            LOG(ERROR, __FUNCTION__, " Listener doesn't exist");
            return telux::common::ErrorCode::INVALID_STATE;
        }

        status = secReportListenerMgr_->deRegisterListener(listener);
        if (status != telux::common::Status::SUCCESS) {
            return telux::common::CommonUtils::toErrorCode(status);
        }

        secReportListenerMgr_->getAvailableListeners(listenerList);
        if (!listenerList.size()) {
            /* If this is the last listener, deregister with lower layers as well
             * to avoid unnecessary updates as there is no application listener. */
            ec = deregisterForSecReports();
            if (ec != telux::common::ErrorCode::SUCCESS) {
                return ec;
            }

            /* clean up resources as there is no listener */
            secReportListenerMgr_ = nullptr;
            listenerExist_        = false;
        }

        return telux::common::ErrorCode::SUCCESS;
    }
}

/*
 * Receives ML analysis report from wcs framework passes to
 * the application listeners.
 */
void WiFiSecurityManagerImpl::onReportAvailable(telux::sec::WiFiSecurityReport report) {

    std::vector<std::weak_ptr<IWiFiReportListener>> listenerList;

    if (getServiceStatus() == ServiceStatus::SERVICE_UNAVAILABLE ||
        getServiceStatus() == ServiceStatus::SERVICE_FAILED) {
        return;
    }

    if (secReportListenerMgr_) {
        secReportListenerMgr_->getAvailableListeners(listenerList);

        for (auto &wp : listenerList) {
            if (auto sp = wp.lock()) {
                sp->onReportAvailable(report);
            }
        }
    }
}

/*
 * Information about the deauthentication attack is passed to the application listeners.
 */
void WiFiSecurityManagerImpl::onDeauthenticationAttack(DeauthenticationInfo deauthenticationInfo) {

    std::vector<std::weak_ptr<IWiFiReportListener>> listenerList;

    if (getServiceStatus() == ServiceStatus::SERVICE_UNAVAILABLE ||
        getServiceStatus() == ServiceStatus::SERVICE_FAILED) {
        return;
    }

    if (secReportListenerMgr_) {
        secReportListenerMgr_->getAvailableListeners(listenerList);

        for (auto &wp : listenerList) {
            if (auto sp = wp.lock()) {
                sp->onDeauthenticationAttack(deauthenticationInfo);
            }
        }
    }
}

/*
 * Indicate the listener to send information to mark AP as trusted.
 */
void WiFiSecurityManagerImpl::isTrustedAP(ApInfo accessPoint, bool &isTrusted) {
    telux::common::ErrorCode ec{};
    std::vector<std::weak_ptr<IWiFiReportListener>> listenerList;

    if (getServiceStatus() == ServiceStatus::SERVICE_UNAVAILABLE ||
        getServiceStatus() == ServiceStatus::SERVICE_FAILED) {
        return;
    }

    if (secReportListenerMgr_) {
        secReportListenerMgr_->getAvailableListeners(listenerList);
        for (auto &wp : listenerList) {
            if (auto sp = wp.lock()) {
                sp->isTrustedAP(accessPoint, isTrusted);
                LOG(DEBUG, __FUNCTION__, " isTrusted: ", isTrusted);
                /* Send the information to the server required to mark AP as trusted. */
                ec = addApToTrustedList(accessPoint, isTrusted);
                if (ec != telux::common::ErrorCode::SUCCESS) {
                    LOG(ERROR, __FUNCTION__, " failed to send if AP is trusted ");
                    continue;
                }
            }
        }
    }
}

/*
 * When a user trusted an access point, it was saved in the WCS database.
 * Retrieve list of all those saved APs and return to the application.
 */
telux::common::ErrorCode WiFiSecurityManagerImpl::getTrustedApList(
    std::vector<ApInfo> &trustedAPList) {

    ApInfo apInfo;

    telux::common::ErrorCode ec;
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    ::securityStub::TrustedAPList response{};

    if (getServiceStatus() == ServiceStatus::SERVICE_UNAVAILABLE ||
        getServiceStatus() == ServiceStatus::SERVICE_FAILED) {
        LOG(ERROR, __FUNCTION__, " Service is unavailable or failed");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    {
        std::lock_guard<std::mutex> lock(WiFiSecurityManagerImpl::operationGuard_);

        reqStatus = stub_->GetTrustedApList(&clientCtx, request, &response);
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " communication error");
            return telux::common::ErrorCode::SYSTEM_ERR;
        }

        ec = static_cast<telux::common::ErrorCode>(response.ec());
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " cant't handle trusted ap input");
            return ec;
        }

        auto aps = response.ap_list();
        for (const securityStub::ApInfo &ap : aps) {
            apInfo.ssid = ap.ssid();
            apInfo.bssid = ap.bssid();
            trustedAPList.push_back(apInfo);
        }
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Delete the given AP from WCS database.
 */
telux::common::ErrorCode WiFiSecurityManagerImpl::removeApFromTrustedList(ApInfo apInfo) {

    telux::common::ErrorCode ec;
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::ApInfo request{};
    ::commonStub::ErrorCodeMsg response{};

    if (getServiceStatus() == ServiceStatus::SERVICE_UNAVAILABLE ||
        getServiceStatus() == ServiceStatus::SERVICE_FAILED) {
        LOG(ERROR, __FUNCTION__, " Service is unavailable or failed");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    {
        std::lock_guard<std::mutex> lock(WiFiSecurityManagerImpl::operationGuard_);

        if (apInfo.ssid.empty() || apInfo.bssid.empty()) {
            LOG(ERROR, __FUNCTION__, " invalid ssid/bssid");
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }

        request.set_ssid(apInfo.ssid);
        request.set_bssid(apInfo.bssid);

        reqStatus = stub_->RemoveApFromTrustedList(&clientCtx, request, &response);
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " communication error");
            return telux::common::ErrorCode::SYSTEM_ERR;
        }

        ec = static_cast<telux::common::ErrorCode>(response.ec());
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " cant't remove ap");
            return ec;
        }
    }

    return ec;
}

/*
 * SSR handling.
 */
void WiFiSecurityManagerImpl::onSecurityServiceStatusChange(ServiceStatus status) {

    if (getServiceStatus() == status) {
        return;
    }
    if (status == ServiceStatus::SERVICE_UNAVAILABLE) {
        LOG(DEBUG, __FUNCTION__, "telux security service is UNAVAILABLE");
        setServiceStatus(status);
    } else {
        LOG(INFO, __FUNCTION__, "telux security service is AVAILABLE");
        auto f = std::async(std::launch::async, [this]() { this->initSync(); }).share();
        LOG(DEBUG, __FUNCTION__, " adding initSync to asyncTaskQueue");
        asyncTaskQueue_.add(f);
    }
}

/*
 * Register with server for getting security reports.
 */
telux::common::ErrorCode WiFiSecurityManagerImpl::registerForSecReports() {

    telux::common::ErrorCode ec;
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    commonStub::ErrorCodeMsg response{};

    reqStatus = stub_->RegisterClientForReport(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " cant't register with server");
        return ec;
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Deregister with server for getting security reports.
 */
telux::common::ErrorCode WiFiSecurityManagerImpl::deregisterForSecReports() {

    telux::common::ErrorCode ec;
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    commonStub::ErrorCodeMsg response{};

    reqStatus = stub_->DeregisterClientForReport(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " cant't deregister with server");
        return ec;
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Update WCS database based on the user response.
 */
telux::common::ErrorCode WiFiSecurityManagerImpl::addApToTrustedList(
    ApInfo accessPoint, bool isTrusted) {

    telux::common::ErrorCode ec;
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::IsTrustedUserResponse request{};
    commonStub::ErrorCodeMsg response{};

    request.set_ssid(accessPoint.ssid);
    request.set_bssid(accessPoint.bssid);
    request.set_is_trusted(isTrusted);

    reqStatus = stub_->SetTrustedAp(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::SYSTEM_ERR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " cant't handle trusted ap input");
        return ec;
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Receives events via simulation event manager framework.
 */
void WiFiSecurityManagerImpl::onEventUpdate(google::protobuf::Any event) {

    ::securityStub::ApInfo isTrustedInfo{};
    ::securityStub::WCSReport wcsReport{};
    ::securityStub::DeauthenticationInfo deauthenticationInfo{};
    ::securityStub::WCSServiceStatus newSrvState{};

    bool isTrusted{false};
    telux::sec::ApInfo accessPoint{};
    telux::sec::WiFiSecurityReport report{};
    telux::sec::DeauthenticationInfo deauthInfo{};
    telux::common::ServiceStatus serviceStatus{};

    if (event.Is<::securityStub::WCSReport>()) {
        event.UnpackTo(&wcsReport);
        report.ssid = wcsReport.ssid();
        report.bssid = wcsReport.bssid();
        report.isConnectedToAP = wcsReport.is_connected_to_ap();
        report.isOpenAP = wcsReport.is_open_ap();
        report.mlAlgorithmAnalysis.threatScore = wcsReport.mlalgo_threat_score();
        report.mlAlgorithmAnalysis.result = static_cast<telux::sec::AnalysisResult>(
            wcsReport.mlalgo_analysis_result());
        report.summoningAnalysis.result = static_cast<telux::sec::AnalysisResult>(
            wcsReport.summoning_analysis_result());
        onReportAvailable(report);
    } else if (event.Is<::securityStub::DeauthenticationInfo>()) {
        event.UnpackTo(&deauthenticationInfo);
        deauthInfo.deauthenticationReason = deauthenticationInfo.deauthentication_reason();
        deauthInfo.didAPInitiateDisconnect = deauthenticationInfo.did_ap_initiate_disconnect();
        deauthInfo.threatScore = deauthenticationInfo.threat_score();
        onDeauthenticationAttack(deauthInfo);
    } else if (event.Is<::securityStub::WCSServiceStatus>()) {
        event.UnpackTo(&newSrvState);
        serviceStatus = static_cast<telux::common::ServiceStatus>(newSrvState.service_status());
        onSecurityServiceStatusChange(serviceStatus);
    } else if (event.Is<::securityStub::ApInfo>()) {
        event.UnpackTo(&isTrustedInfo);
        accessPoint.ssid = isTrustedInfo.ssid();
        accessPoint.bssid = isTrustedInfo.bssid();
        isTrustedAP(accessPoint, isTrusted);
    } else {
    }
}

}  // End of namespace sec
}  // End of namespace telux

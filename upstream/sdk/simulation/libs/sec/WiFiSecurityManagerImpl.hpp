/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef WIFISECURITYMANAGERIMPL_HPP
#define WIFISECURITYMANAGERIMPL_HPP

#include "common/CommonUtils.hpp"
#include "common/TaskDispatcher.hpp"
#include "common/ListenerManager.hpp"
#include "common/AsyncTaskQueue.hpp"

#include "common/event-manager/ClientEventManager.hpp"

#include "protos/proto-src/security_simulation.grpc.pb.h"
#include <grpcpp/grpcpp.h>

#include <telux/sec/WiFiSecurityManager.hpp>

namespace telux {
namespace sec {

using namespace telux::common;

class WiFiSecurityManagerImpl;

struct WiFiPrivateCookie {
    std::weak_ptr<WiFiSecurityManagerImpl> wsManagerImpl;
};

class WiFiSecurityManagerImpl : public IWiFiSecurityManager,
                                public IWiFiReportListener,
                                public telux::common::IEventListener,
                                public std::enable_shared_from_this<WiFiSecurityManagerImpl> {

 public:
    WiFiSecurityManagerImpl();
    ~WiFiSecurityManagerImpl();

    WiFiSecurityManagerImpl(const WiFiSecurityManagerImpl &)            = delete;
    WiFiSecurityManagerImpl &operator=(const WiFiSecurityManagerImpl &) = delete;

    telux::common::Status init(telux::common::InitResponseCb callback);
    void cleanup();

    /* Security service availability management */
    void initSync();

    /* IWiFiSecurityManager overrides */

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::ErrorCode registerListener(
        std::weak_ptr<IWiFiReportListener> reportListener) override;

    telux::common::ErrorCode deregisterListener(
        std::weak_ptr<IWiFiReportListener> reportListener) override;

    telux::common::ErrorCode getTrustedApList(std::vector<ApInfo> &trustedAPList) override;

    telux::common::ErrorCode removeApFromTrustedList(ApInfo apInfo) override;

    telux::common::ErrorCode registerListener(
        std::weak_ptr<IServiceStatusListener> listener) override;

    telux::common::ErrorCode deregisterListener(
        std::weak_ptr<IServiceStatusListener> listener) override;

    /* IQmiSecListener overrides */

    void onSecurityServiceStatusChange(ServiceStatus status);

    /* IWiFiReportListener overrides */

    void onReportAvailable(telux::sec::WiFiSecurityReport report) override;

    void onDeauthenticationAttack(DeauthenticationInfo deauthenticationInfo) override;

    void isTrustedAP(ApInfo accessPoint, bool &isTrusted) override;

    void onEventUpdate(google::protobuf::Any event) override;

    /* Protect against:
     * (a) concurrent reporting & WiFiSecurityManagerImpl destruction
     * (b) concurrent listener registration and deregistration */
    static std::mutex operationGuard_;

 private:
    std::mutex mutex_;
    InitResponseCb initCb_;
    /* prevents invalid functional flow */
    bool listenerExist_       = false;
    bool isInitsyncTriggered_ = false;

    telux::common::AsyncTaskQueue<void> asyncTaskQueue_;
    ServiceStatus serviceStatus_{ServiceStatus::SERVICE_UNAVAILABLE};

    std::shared_ptr<telux::common::ListenerManager<
        IServiceStatusListener>> serviceStatusListenerMgr_;
    std::shared_ptr<telux::common::ListenerManager<
        IWiFiReportListener>> secReportListenerMgr_;

    uint32_t ss_ready_delay_ = 0;
    ServiceStatus ss_service_status_;
    const char * const WCS_FILTER = "wcs";
    const char * const WCS_API_JSON_FILE = "api/sec/IWiFiSecurityManager.json";
    ClientEventManager &clientEventMgr_;
    std::unique_ptr<::securityStub::SecurityWCSService::Stub> stub_;

    void setServiceStatus(ServiceStatus status);
    telux::common::ServiceStatus convertServiceStatus();
    telux::common::ErrorCode addApToTrustedList(ApInfo accessPoint, bool isTrusted);
    telux::common::ErrorCode registerForSecReports();
    telux::common::ErrorCode deregisterForSecReports();
};

}  // End of namespace sec
}  // End of namespace telux

#endif  // WIFISECURITYMANAGERIMPL_HPP
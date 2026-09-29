/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CELLULARSECURITYMANAGERIMPL_HPP
#define CELLULARSECURITYMANAGERIMPL_HPP

#include "connsec.h"

#include "common/CommonUtils.hpp"
#include "common/TaskDispatcher.hpp"
#include "common/ListenerManager.hpp"

#include "common/event-manager/EventParserUtil.hpp"
#include "common/event-manager/ClientEventManager.hpp"

#include "protos/proto-src/security_simulation.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include "libs/common/JsonParser.hpp"

#include <telux/sec/CellularSecurityManager.hpp>

namespace telux {
namespace sec {

class CellularSecurityManagerImpl : public ICellularSecurityManager,
     public telux::common::IEventListener,
     public std::enable_shared_from_this<CellularSecurityManagerImpl> {

 public:
    CellularSecurityManagerImpl();
    ~CellularSecurityManagerImpl();

    CellularSecurityManagerImpl(const CellularSecurityManagerImpl &)            = delete;
    CellularSecurityManagerImpl &operator=(const CellularSecurityManagerImpl &) = delete;

    telux::common::ErrorCode init();

    telux::common::ErrorCode registerListener(
        std::weak_ptr<ICellularScanReportListener> listener) override;

    telux::common::ErrorCode deRegisterListener(
        std::weak_ptr<ICellularScanReportListener> listener) override;

    telux::common::ErrorCode getCurrentSessionStats(SessionStats &sessionStats) override;

    int32_t rawReportHandler(::securityStub::CCSReport ccsReport, void *cookie);

    void ssrHandler(ssgccs_state_t ccsState, void *cookie);

    void onEventUpdate(google::protobuf::Any event) override;

    /* Protect against:
     * (a) concurrent reporting & CellularSecurityManagerImpl destruction
     * (b) concurrent listener registration and deregistration */
    std::mutex operationGuard_;

    /* Set to true to indicate - CellularSecurityManagerImpl is marked for destruction */
    std::atomic<bool> exitNow_;

 private:
    telux::common::ServiceStatus serviceStatus_;
    const uint32_t LAST_INDICATION_QUEUE_SIZE = 5;
    telux::common::ErrorCode reconnectCCS();
    telux::common::ErrorCode disconnectCCS(bool isExiting_);

    void populateThreatTypes(uint32_t category, std::vector<CellularThreatType> &threats);

    void deliverReportOrSSREvent(bool isReport, telux::common::ServiceStatus serviceStatus,
        CellularSecurityReport finalReport, EnvironmentInfo environmentInfo, void *cookie);

    void populatePolicy(uint32_t policyActed, ActionType &actionType);

    void populateRAT(ssgccs_radio_enum_t radio, RATType &ratType);

    void populateEnvironmentState(
        ssgccs_environmental_state_t envState, EnvironmentState &environmentState);

    /* prevents invalid functional flow */
    bool listenerExist_ = false;

    /* Dispatches reports and SSR events to the registered listeners */
    std::shared_ptr<telux::common::TaskDispatcher> reportAndSSRDispatcher_;

    std::shared_ptr<telux::common::ListenerManager<ICellularScanReportListener>> csListenerMgr_;

    const char * const CCS_API_JSON_FILE = "api/sec/ICellularSecurityManager.json";
    const char * const CCS_FILTER = "ccs";
    ClientEventManager &clientEventMgr_;
    std::unique_ptr<::securityStub::SecurityCCSService::Stub> stub_;
};

}  // End of namespace sec
}  // End of namespace telux

#endif  // CELLULARSECURITYMANAGERIMPL_HPP

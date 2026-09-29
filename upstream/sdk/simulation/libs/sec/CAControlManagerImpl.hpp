/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CACONTROLMANAGERIMPL_HPP
#define CACONTROLMANAGERIMPL_HPP

#include <signal.h>
#include <time.h>

#include "common/CommonUtils.hpp"
#include "common/TaskDispatcher.hpp"
#include "common/ListenerManager.hpp"

#include "common/event-manager/EventParserUtil.hpp"
#include "common/event-manager/ClientEventManager.hpp"

#include "protos/proto-src/security_simulation.grpc.pb.h"
#include <grpcpp/grpcpp.h>

#include <telux/sec/CAControlManager.hpp>

namespace telux {
namespace sec {

typedef struct {
    uint32_t nistp256;
    uint32_t bp256r1;
    uint32_t sm2;
    uint32_t nistp384;
    uint32_t bp384r1;
} MVM_CAPACITY;

typedef struct {
    uint32_t nistp256;
    uint32_t bp256r1;
    uint32_t sm2;
    uint32_t nistp384;
    uint32_t bp384r1;
} MVM_STATS_MSG_COUNT;

struct PrivateCookie {
    std::weak_ptr<ICAControlManager> caControlManagerImpl;
};

class CAControlManagerImpl : public ICAControlManager,
                             public telux::common::IEventListener,
                             public std::enable_shared_from_this<CAControlManagerImpl> {

 public:
    CAControlManagerImpl();
    ~CAControlManagerImpl();

    CAControlManagerImpl(const CAControlManagerImpl &)            = delete;
    CAControlManagerImpl &operator=(const CAControlManagerImpl &) = delete;

    telux::common::ErrorCode init();

    telux::common::ErrorCode registerListener(
        std::weak_ptr<ICAControlManagerListener> listener) override;

    telux::common::ErrorCode deRegisterListener(
        std::weak_ptr<ICAControlManagerListener> listener) override;

    telux::common::ErrorCode startMonitoring(LoadConfig loadConfig) override;

    telux::common::ErrorCode stopMonitoring() override;

    telux::common::ErrorCode getCapacity(CACapacity &capacity) override;

    static void sendCapacityUpdate(MVM_CAPACITY capacity, void *cookie);

    static void sendLoadUpdate(union sigval sigVal);

    static void deliverUpdatedData(
        bool isLoad, CACapacity newCapacity, MVM_STATS_MSG_COUNT counts, void *cookie);

    void onEventUpdate(google::protobuf::Any event) override;

    /* Protect against:
     * (a) concurrent load/capacity reporting & CAControlManagerImpl destruction
     * (b) concurrent listener registration and deregistration */
    static std::mutex operationGuard_;

    /* Set to true to indicate - CAControlManagerImpl is marked for destruction */
    static std::atomic<bool> exitNow_;

    /* prevents invalid functional flow */
    static bool listenerExist_;

 private:
    /* Uniquely identifies timer */
    timer_t timerId_ = 0;

    PrivateCookie *privateCookie_;

    /* protects against uninitialized resources destruction */
    bool initComplete_ = false;

    /* last load readings */
    CALoad cachedLastMsgCounts_{};

    /* true if the very first load has been cached locally */
    bool msgCountInitialized_ = false;

    /* Dispatches load and capacity to the registered listeners */
    std::shared_ptr<telux::common::TaskDispatcher> newStatsDispatcher_;

    std::shared_ptr<telux::common::ListenerManager<
        ICAControlManagerListener>> caCtrlListenerMgr_;

    void cacheMessageCounts(MVM_STATS_MSG_COUNT messageCounts);

    const char * const CALC_FILTER = "calc";
    ClientEventManager &clientEventMgr_;
    static std::unique_ptr<::securityStub::SecurityCALCService::Stub> stub_;

    int mvm_capacity_unreg_cb();
    int mvm_stats_deinit();
};

}  // End of namespace sec
}  // End of namespace telux

#endif  // CACONTROLMANAGERIMPL_HPP

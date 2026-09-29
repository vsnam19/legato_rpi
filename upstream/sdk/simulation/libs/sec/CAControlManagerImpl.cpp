/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <climits>

#include "common/CommonUtils.hpp"

#include "CryptoAcceleratorUtils.hpp"
#include "CAControlManagerImpl.hpp"

namespace telux {
namespace sec {

std::atomic<bool> CAControlManagerImpl::exitNow_;

std::mutex CAControlManagerImpl::operationGuard_;

bool CAControlManagerImpl::listenerExist_;

std::unique_ptr<::securityStub::SecurityCALCService::Stub> CAControlManagerImpl::stub_;

CAControlManagerImpl::CAControlManagerImpl()
    : clientEventMgr_(ClientEventManager::getInstance()) {
    exitNow_ = false;
}

/*
 * If the resources are not deallocated as part of normal functional flow,
 * deallocate them here.
 */
CAControlManagerImpl::~CAControlManagerImpl() {
    LOG(DEBUG, __FUNCTION__);

    if (!initComplete_) {
        return;
    }

    std::lock_guard<std::mutex> lock(CAControlManagerImpl::operationGuard_);

    exitNow_ = true;

    if (listenerExist_) {
        /* As the listener exist, these resources exist. Clean up them. */
        timer_delete(timerId_);
    }

    if(privateCookie_) {
        delete privateCookie_;
        privateCookie_ = nullptr;
    }
    mvm_stats_deinit();
}

/*
 * Allocates resources and initializes them as applicable.
 */
telux::common::ErrorCode CAControlManagerImpl::init() {

    telux::common::ErrorCode ec;
    telux::common::Status status;

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    commonStub::ErrorCodeMsg response{};

    try {
        caCtrlListenerMgr_ = std::make_shared<
            telux::common::ListenerManager<ICAControlManagerListener>>();
    } catch (const std::exception &e) {
        LOG(ERROR, __FUNCTION__, " can't create ICAControlManagerListener");
        return telux::common::ErrorCode::NO_MEMORY;
    }

    status = clientEventMgr_.registerListener(shared_from_this(), CALC_FILTER);
    if ((status != telux::common::Status::SUCCESS) &&
        (status != telux::common::Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, " can't register with ClientEventManager");
        return telux::common::CommonUtils::toErrorCode(status);
    }

    stub_ = CommonUtils::getGrpcStub<securityStub::SecurityCALCService>();

    reqStatus = stub_->Init(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " cant't register with server");
        return ec;
    }

    listenerExist_ = false;
    initComplete_  = true;
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * 1. Registers listener to receive capacity updates. Register only once
 *    with libmvm. For subsequent registrations, new listener is just appended
 *    to the internal list.
 * 2. Create timer but don't start it. Upon timer expiration, current load will
 *    be fetched from libmvm and passed to the registered application's listener
 *    by dispatcher.
 */
telux::common::ErrorCode CAControlManagerImpl::registerListener(
    std::weak_ptr<ICAControlManagerListener> listener) {

    int res;
    telux::common::ErrorCode ec;
    struct sigevent sigEvent {};

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    ::commonStub::ErrorCodeMsg response{};

    telux::common::Status status;
    std::vector<std::weak_ptr<ICAControlManagerListener>> listenerList;

    {
        std::lock_guard<std::mutex> lock(CAControlManagerImpl::operationGuard_);

        caCtrlListenerMgr_->getAvailableListeners(listenerList);
        if (listenerList.size()) {
            /* at-least 1 listener already registered, therefore, just add this
             * new listener to the list. */
            status = caCtrlListenerMgr_->registerListener(listener);
            if (status != telux::common::Status::SUCCESS) {
                return telux::common::CommonUtils::toErrorCode(status);
            }

            return telux::common::ErrorCode::SUCCESS;
        }

        try {
            newStatsDispatcher_ = std::make_shared<telux::common::TaskDispatcher>();
        } catch (const std::exception &e) {
            LOG(ERROR, __FUNCTION__, " can't create TaskDispatcher");
            return telux::common::ErrorCode::NO_MEMORY;
        }

        /* No listener exist, allocate resources and register with required libraries. */
        privateCookie_ = new (std::nothrow) PrivateCookie();
        if (!privateCookie_) {
            LOG(ERROR, __FUNCTION__, " can't allocate PrivateCookie");
            ec = telux::common::ErrorCode::NO_MEMORY;
            goto err1;
        }

        privateCookie_->caControlManagerImpl = shared_from_this();

        reqStatus = stub_->RegisterClient(&clientCtx, request, &response);
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " communication error");
            return telux::common::ErrorCode::TRANSPORT_ERROR;
        }

        ec = static_cast<telux::common::ErrorCode>(response.ec());
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't register, err ", static_cast<int>(ec));
            goto err2;
        }

        sigEvent.sigev_notify          = SIGEV_THREAD;
        sigEvent.sigev_notify_function = &sendLoadUpdate;
        sigEvent.sigev_value.sival_ptr = privateCookie_;
        res                            = timer_create(CLOCK_BOOTTIME, &sigEvent, &timerId_);
        if (res < 0) {
            LOG(ERROR, __FUNCTION__, " can't create timer, err ", static_cast<int>(errno));
            ec = telux::common::ErrorCode::NO_RESOURCES;
            goto err3;
        }

        status = caCtrlListenerMgr_->registerListener(listener);
        if (status != telux::common::Status::SUCCESS) {
            ec = telux::common::CommonUtils::toErrorCode(status);
            goto err4;
        }

        listenerExist_ = true;
        return telux::common::ErrorCode::SUCCESS;

    err4:
        timer_delete(timerId_);
    err3:
        mvm_capacity_unreg_cb();
    err2:
        delete privateCookie_;
        privateCookie_ = nullptr;
    err1:
        newStatsDispatcher_ = nullptr;
    }

    return ec;
}

/*
 * Deregisters given listener previously registered with registerListener().
 */
telux::common::ErrorCode CAControlManagerImpl::deRegisterListener(
    std::weak_ptr<ICAControlManagerListener> listener) {

    int ret;

    telux::common::Status status;
    std::vector<std::weak_ptr<ICAControlManagerListener>> listenerList;

    {
        std::lock_guard<std::mutex> lock(CAControlManagerImpl::operationGuard_);

        status = caCtrlListenerMgr_->deRegisterListener(listener);
        if (status != telux::common::Status::SUCCESS) {
            return telux::common::CommonUtils::toErrorCode(status);
        }

        caCtrlListenerMgr_->getAvailableListeners(listenerList);
        if (!listenerList.size()) {
            /* If this is the last listener, deregister with lower layers as well
             * to avoid unnecessary updates as there is no application listener. */
            ret = mvm_capacity_unreg_cb();
            if (ret != 0) {
                LOG(ERROR, __FUNCTION__, " can't deregister, err ", static_cast<int>(ret));
                /* don't treate this as fatal, continue further */
            }

            /* clean up resources as there is no listener */
            timer_delete(timerId_);
            newStatsDispatcher_ = nullptr;
            delete privateCookie_;
            privateCookie_ = nullptr;
            listenerExist_ = false;
        }
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Arm the timer to expire at the interval specified by the application.
 * Upon expiration, timer is re-armed again automatically.
 */
telux::common::ErrorCode CAControlManagerImpl::startMonitoring(LoadConfig loadConfig) {

    int ret;
    struct itimerspec timerSpecs {};

    {
        std::lock_guard<std::mutex> lock(CAControlManagerImpl::operationGuard_);

        if (!listenerExist_) {
            LOG(ERROR, __FUNCTION__, " no listener");
            return telux::common::ErrorCode::INVALID_STATE;
        }

        /* Set initial timer expiry least possible valid value so as to cache very first
         * load reading as soon as possible. 10 nano seconds is set as initial timeout. */
        timerSpecs.it_value.tv_sec  = 0;
        timerSpecs.it_value.tv_nsec = 10;

        /* Set periodic load reporting interval as specified by the application */
        timerSpecs.it_interval.tv_sec  = loadConfig.calculationInterval / 1000;
        timerSpecs.it_interval.tv_nsec = (loadConfig.calculationInterval % 1000) * 1000000;

        ret = timer_settime(timerId_, 0, &timerSpecs, NULL);
        if (ret < 0) {
            LOG(ERROR, __FUNCTION__, " can't start timer, err ", static_cast<int>(errno));
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Disable the timer for load reporting.
 */
telux::common::ErrorCode CAControlManagerImpl::stopMonitoring() {

    int ret;
    struct itimerspec timerSpecs {};

    {
        std::lock_guard<std::mutex> lock(CAControlManagerImpl::operationGuard_);

        if (!listenerExist_) {
            LOG(ERROR, __FUNCTION__, " no listener");
            return telux::common::ErrorCode::INVALID_STATE;
        }

        timerSpecs.it_value.tv_sec  = 0;
        timerSpecs.it_value.tv_nsec = 0;

        ret = timer_settime(timerId_, 0, &timerSpecs, NULL);
        if (ret < 0) {
            LOG(ERROR, __FUNCTION__, " can't stop timer, err ", static_cast<int>(errno));
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Gives current capacity.
 */
telux::common::ErrorCode CAControlManagerImpl::getCapacity(CACapacity &capacity) {

    telux::common::ErrorCode ec;

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    ::securityStub::Capacity response{};

    reqStatus = stub_->GetCapacity(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't get capacity, err ", static_cast<int>(ec));
        return ec;
    }

    capacity.sm2     = response.sm2();
    capacity.nist256 = response.nist256();
    capacity.nist384 = response.nist384();
    capacity.bp256   = response.bp256();
    capacity.bp384   = response.bp384();

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Callback invoked by libmvm to pass the updated capacity. This update is
 * provided to application through dispatcher.
 */
void CAControlManagerImpl::sendCapacityUpdate(MVM_CAPACITY capacity, void *cookie) {

    CACapacity newCapacity{};

    newCapacity.sm2     = capacity.sm2;
    newCapacity.nist256 = capacity.nistp256;
    newCapacity.nist384 = capacity.nistp384;
    newCapacity.bp256   = capacity.bp256r1;
    newCapacity.bp384   = capacity.bp384r1;

    CAControlManagerImpl::deliverUpdatedData(false, newCapacity, MVM_STATS_MSG_COUNT(), cookie);
}

/*
 * Timer expired, fetch message counts and pass it to the application.
 */
void CAControlManagerImpl::sendLoadUpdate(union sigval sigVal) {

    MVM_STATS_MSG_COUNT counts{};
    telux::common::ErrorCode ec;

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    ::securityStub::LoadCount response{};

    reqStatus = stub_->GetOperationsCount(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return;
    }

    ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't get load, err ", static_cast<int>(ec));
        /* nothing can be done, just return */
        return;
    }

    counts.nistp256 = response.nist256();
    counts.bp256r1 = response.bp256();
    counts.sm2 = response.sm2();
    counts.nistp384 = response.nist384();
    counts.bp384r1 = response.bp384();

    CAControlManagerImpl::deliverUpdatedData(true, CACapacity(), counts, sigVal.sival_ptr);
}

/*
 * Common helper method to pass message counts and capacity to the application.
 */
void CAControlManagerImpl::deliverUpdatedData(
    bool isLoad, CACapacity newCapacity, MVM_STATS_MSG_COUNT msgCounts, void *cookie) {

    CALoad currentLoad{};
    PrivateCookie *privateCookie;
    std::shared_ptr<CAControlManagerImpl> caCtrlMgr;
    std::vector<std::weak_ptr<ICAControlManagerListener>> listenerList;

    {
        std::lock_guard<std::mutex> lock(CAControlManagerImpl::operationGuard_);

        if (!cookie) {
            LOG(DEBUG, __FUNCTION__, " missing cookie");
            return;
        }

        if (exitNow_ || !listenerExist_) {
            /* CAControlManagerImpl has been destructed or no listener */
            LOG(DEBUG, __FUNCTION__, " exiting!");
            return;
        }

        privateCookie = static_cast<PrivateCookie *>(cookie);

        auto tmp = (privateCookie->caControlManagerImpl).lock();
        if (!tmp) {
            /* application released CAControlManagerImpl shared pointer instance */
            LOG(DEBUG, __FUNCTION__, " CAControlManagerImpl doesn't exist");
            return;
        }

        caCtrlMgr = std::dynamic_pointer_cast<CAControlManagerImpl>(tmp);

        caCtrlMgr->caCtrlListenerMgr_->getAvailableListeners(listenerList);
        if (!listenerList.size()) {
            LOG(ERROR, __FUNCTION__, " no listener");
            return;
        }

        if (isLoad) {
            if (!caCtrlMgr->msgCountInitialized_) {
                /* If this is the very first count reading, cache it for subsequent use */
                caCtrlMgr->cacheMessageCounts(msgCounts);
                caCtrlMgr->msgCountInitialized_ = true;
                return;
            }

            /* Calculate load values as observed in the set time window */
            if (msgCounts.sm2 < caCtrlMgr->cachedLastMsgCounts_.sm2) {
                /* handle roll-over */
                currentLoad.sm2 = msgCounts.sm2 + (UINT_MAX - caCtrlMgr->cachedLastMsgCounts_.sm2);
            } else {
                currentLoad.sm2 = msgCounts.sm2 - caCtrlMgr->cachedLastMsgCounts_.sm2;
            }

            if (msgCounts.nistp256 < caCtrlMgr->cachedLastMsgCounts_.nist256) {
                currentLoad.nist256
                    = msgCounts.nistp256 + (UINT_MAX - caCtrlMgr->cachedLastMsgCounts_.nist256);
            } else {
                currentLoad.nist256 = msgCounts.nistp256 - caCtrlMgr->cachedLastMsgCounts_.nist256;
            }

            if (msgCounts.nistp384 < caCtrlMgr->cachedLastMsgCounts_.nist384) {
                currentLoad.nist384
                    = msgCounts.nistp384 + (UINT_MAX - caCtrlMgr->cachedLastMsgCounts_.nist384);
            } else {
                currentLoad.nist384 = msgCounts.nistp384 - caCtrlMgr->cachedLastMsgCounts_.nist384;
            }

            if (msgCounts.bp256r1 < caCtrlMgr->cachedLastMsgCounts_.bp256) {
                currentLoad.bp256
                    = msgCounts.bp256r1 + (UINT_MAX - caCtrlMgr->cachedLastMsgCounts_.bp256);
            } else {
                currentLoad.bp256 = msgCounts.bp256r1 - caCtrlMgr->cachedLastMsgCounts_.bp256;
            }

            if (msgCounts.bp384r1 < caCtrlMgr->cachedLastMsgCounts_.bp384) {
                currentLoad.bp384
                    = msgCounts.bp384r1 + (UINT_MAX - caCtrlMgr->cachedLastMsgCounts_.bp384);
            } else {
                currentLoad.bp384 = msgCounts.bp384r1 - caCtrlMgr->cachedLastMsgCounts_.bp384;
            }
        }

        for (size_t x = 0; x < listenerList.size(); x++) {
            if (auto sp
                = std::dynamic_pointer_cast<ICAControlManagerListener>(listenerList[x].lock())) {
                caCtrlMgr->newStatsDispatcher_->submitTask([=] {
                    if (isLoad) {
                        sp->onLoadUpdate(currentLoad);
                    } else {
                        sp->onCapacityUpdate(newCapacity);
                    }
                });
                continue;
            }

            LOG(ERROR, __FUNCTION__, " no listener");
            /* don't treat as error, application just released listener */
        }

        /* Update the cache to contain latest message counts */
        if (isLoad) {
            caCtrlMgr->cacheMessageCounts(msgCounts);
        }
    }
}

/*
 * Cache the latest message counts.
 */
void CAControlManagerImpl::cacheMessageCounts(MVM_STATS_MSG_COUNT msgCounts) {
    cachedLastMsgCounts_.sm2     = msgCounts.sm2;
    cachedLastMsgCounts_.nist256 = msgCounts.nistp256;
    cachedLastMsgCounts_.nist384 = msgCounts.nistp384;
    cachedLastMsgCounts_.bp256   = msgCounts.bp256r1;
    cachedLastMsgCounts_.bp384   = msgCounts.bp384r1;
}

/*
 * Receives events via simulation event manager framework.
 */
void CAControlManagerImpl::onEventUpdate(google::protobuf::Any event) {

    MVM_CAPACITY mvmCapacity{};
    ::securityStub::Capacity capacity{};

    if (event.Is<::securityStub::Capacity>()) {
        event.UnpackTo(&capacity);
        mvmCapacity.nistp256 = capacity.nist256();
        mvmCapacity.bp256r1 = capacity.bp256();
        mvmCapacity.sm2 = capacity.sm2();
        mvmCapacity.nistp384 = capacity.nist384();
        mvmCapacity.bp384r1 = capacity.bp384();
        sendCapacityUpdate(mvmCapacity, privateCookie_);
    }
}

int CAControlManagerImpl::mvm_capacity_unreg_cb() {

    telux::common::ErrorCode ec;

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    ::commonStub::ErrorCodeMsg response{};

    reqStatus = stub_->DeregisterClient(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return -1;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't deregister, err ", static_cast<int>(ec));
        return -1;
    }

    return 0;
}

int CAControlManagerImpl::mvm_stats_deinit() {

    telux::common::ErrorCode ec;

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    ::commonStub::ErrorCodeMsg response{};

    reqStatus = stub_->DeInit(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return -1;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't deinit, err ", static_cast<int>(ec));
        return -1;
    }

    return 0;
}

}  // End of namespace sec
}  // End of namespace telux

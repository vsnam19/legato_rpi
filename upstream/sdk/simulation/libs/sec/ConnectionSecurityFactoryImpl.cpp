/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

#include "ConnectionSecurityFactoryImpl.hpp"

namespace telux {
namespace sec {

ConnectionSecurityFactoryImpl::ConnectionSecurityFactoryImpl() {
}

ConnectionSecurityFactoryImpl::~ConnectionSecurityFactoryImpl() {
    LOG(DEBUG, __FUNCTION__);
}

ConnectionSecurityFactory::ConnectionSecurityFactory() {
}

ConnectionSecurityFactory::~ConnectionSecurityFactory() {
}

ConnectionSecurityFactory &ConnectionSecurityFactoryImpl::getInstance() {
    static ConnectionSecurityFactoryImpl instance;
    return instance;
}

ConnectionSecurityFactory &ConnectionSecurityFactory::getInstance() {
    return ConnectionSecurityFactoryImpl::getInstance();
}

/**
 * Gets a CellularSecurityManagerImpl instance.
 *
 * @return Shared pointer to the CellularSecurityManagerImpl object.
 */
std::shared_ptr<ICellularSecurityManager> ConnectionSecurityFactoryImpl::getCellularSecurityManager(
    telux::common::ErrorCode &ec) {

    std::shared_ptr<CellularSecurityManagerImpl> conSecMgr;

    std::lock_guard<std::mutex> lock(secFactoryGuard_);

    conSecMgr = conSecMgr_.lock();
    if (conSecMgr) {
        ec = telux::common::ErrorCode::SUCCESS;
        return conSecMgr;
    }

    try {
        conSecMgr = std::make_shared<CellularSecurityManagerImpl>();
    } catch (const std::exception &e) {
        ec = telux::common::ErrorCode::NO_MEMORY;
        LOG(ERROR, __FUNCTION__, " can't create CellularSecurityManagerImpl");
        return nullptr;
    }

    ec = conSecMgr->init();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't init ICellularSecurityManager");
        return nullptr;
    }

    /* Save manager reference */
    conSecMgr_ = conSecMgr;

    ec = telux::common::ErrorCode::SUCCESS;
    return conSecMgr;
}

/**
 * Gets a WiFiSecurityManagerImpl instance.
 *
 * @return Shared pointer to the WiFiSecurityManagerImpl object.
 */
std::shared_ptr<IWiFiSecurityManager> ConnectionSecurityFactoryImpl::getWiFiSecurityManager(
    telux::common::ErrorCode &ec) {

    LOG(WARNING, __FUNCTION__, " Deprecated API used");
    ec = telux::common::ErrorCode::NOT_SUPPORTED;
    return nullptr;
}

/**
 * Gets a WiFiSecurityManagerImpl instance.
 *
 * @return Shared pointer to the WiFiSecurityManagerImpl object.
 */
std::shared_ptr<IWiFiSecurityManager> ConnectionSecurityFactoryImpl::getWiFiSecurityManager(
    telux::common::InitResponseCb callback) {

    std::lock_guard<std::mutex> lock(secFactoryGuard_);

    // Creating the createAndInitCb
    std::function<std::shared_ptr<IWiFiSecurityManager>(telux::common::InitResponseCb)> createAndInit
        = [](telux::common::InitResponseCb initCb) -> std::shared_ptr<IWiFiSecurityManager> {
        std::shared_ptr<WiFiSecurityManagerImpl> wifiConSecMgr;
        try {
            wifiConSecMgr = std::make_shared<WiFiSecurityManagerImpl>();
            if (wifiConSecMgr && (wifiConSecMgr->init(initCb) == telux::common::Status::SUCCESS)) {
                return wifiConSecMgr;
            }
        } catch (const std::exception &e) {
            LOG(ERROR, __FUNCTION__, " can't create WiFiSecurityManagerImpl");
        }
        return nullptr;
    };

    std::string type = std::string("WiFiSecurity manager");
    LOG(DEBUG, __FUNCTION__, " requesting ", type.c_str(), ", cb ", &initCompleteCallbacks_);

    auto mgrWiFiSecurity = telux::common::FactoryHelper::getManager<IWiFiSecurityManager>(
        type, wifiConSecMgr_, initCompleteCallbacks_, callback, createAndInit);

    return mgrWiFiSecurity;
}

}  // end namespace sec
}  // end namespace telux

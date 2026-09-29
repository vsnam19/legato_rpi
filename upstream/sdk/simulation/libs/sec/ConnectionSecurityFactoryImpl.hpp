/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CONNECTIONSECURITYFACTORYIMPL_HPP
#define CONNECTIONSECURITYFACTORYIMPL_HPP

#include <mutex>

#include <telux/sec/ConnectionSecurityFactory.hpp>

#include "common/FactoryHelper.hpp"
#include "CellularSecurityManagerImpl.hpp"
#include "WiFiSecurityManagerImpl.hpp"

namespace telux {
namespace sec {

class ConnectionSecurityFactoryImpl : public ConnectionSecurityFactory,
                                      public telux::common::FactoryHelper {

 public:
    static ConnectionSecurityFactory &getInstance();

    std::shared_ptr<ICellularSecurityManager> getCellularSecurityManager(
        telux::common::ErrorCode &ec) override;

    std::shared_ptr<IWiFiSecurityManager> getWiFiSecurityManager(
        telux::common::ErrorCode &ec) override;

    std::shared_ptr<IWiFiSecurityManager> getWiFiSecurityManager(
        telux::common::InitResponseCb callback) override;

 private:
    std::mutex secFactoryGuard_;
    std::weak_ptr<CellularSecurityManagerImpl> conSecMgr_;
    std::weak_ptr<IWiFiSecurityManager> wifiConSecMgr_;
    std::vector<telux::common::InitResponseCb> initCompleteCallbacks_;

    ConnectionSecurityFactoryImpl();
    ~ConnectionSecurityFactoryImpl();
};

}  // End of namespace sec
}  // End of namespace telux

#endif  // CONNECTIONSECURITYFACTORYIMPL_HPP

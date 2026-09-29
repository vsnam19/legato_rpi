/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SECURITYFACTORYIMPL_HPP
#define SECURITYFACTORYIMPL_HPP

#include <mutex>

#include <telux/sec/SecurityFactory.hpp>

#include "CryptoManagerImpl.hpp"
#include "CryptoAcceleratorManagerImpl.hpp"
#include "CAControlManagerImpl.hpp"
#include "RandomNumberManagerImpl.hpp"

namespace telux {
namespace sec {

class SecurityFactoryImpl : public SecurityFactory {

 public:
    static SecurityFactory &getInstance();

    std::shared_ptr<ICryptoManager> getCryptoManager(telux::common::ErrorCode &ec) override;

    std::shared_ptr<ICryptoAcceleratorManager> getCryptoAcceleratorManager(
        telux::common::ErrorCode &ec, Mode mode,
        std::weak_ptr<ICryptoAcceleratorListener> caListener) override;

    std::shared_ptr<ICAControlManager> getCAControlManager(telux::common::ErrorCode &ec) override;

    std::shared_ptr<IRandomNumberManager> getRandomNumberManager(
        RNGSource generatorSource, telux::common::ErrorCode &ec) override;

 private:
    std::mutex secFactoryGuard_;
    telux::sec::Mode cryptAccelMode_;
    std::weak_ptr<CryptoManagerImpl> cryptMgr_;
    std::weak_ptr<CryptoAcceleratorManagerImpl> cryptAccelMgr_;
    std::weak_ptr<ICryptoAcceleratorListener> caListener_;
    std::weak_ptr<CAControlManagerImpl> caCtrlMgr_;

    SecurityFactoryImpl();
    ~SecurityFactoryImpl();
};

}  // End of namespace sec
}  // End of namespace telux

#endif  // SECURITYFACTORYIMPL_HPP

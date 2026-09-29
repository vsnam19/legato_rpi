/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <thread>
/*
 * Steps to register listener for load and capacity updates are:
 * 1. Get SecurityFactory instance.
 * 2. Get an ICAControlManager instance from SecurityFactory.
 * 3. Define listener that implements ICAControlManagerListener.
 * 4. Register listener using registerListener().
 * 5. Start monitoring load by defining parameters and calling startMonitoring().
 * 6. When use-case is complete, stop monitoring using stopMonitoring().
 * 7. Finally, release listener using deRegisterListener().
 */

#include <iostream>

#include <telux/sec/SecurityFactory.hpp>

/* Step - 3 */
class StatsListener : public telux::sec::ICAControlManagerListener {

    void onCapacityUpdate(telux::sec::CACapacity newCapacity) {
        std::cout << "onCapacityUpdate()" << std::endl;
        std::cout << "sm2     : " << newCapacity.sm2 << std::endl;
        std::cout << "nist256 : " << newCapacity.nist256 << std::endl;
        std::cout << "nist384 : " << newCapacity.nist384 << std::endl;
        std::cout << "bp256   : " << newCapacity.bp256 << std::endl;
        std::cout << "bp384   : " << newCapacity.bp384 << std::endl;
    }

    void onLoadUpdate(telux::sec::CALoad currentLoad) {
        std::cout << "onLoadUpdate()" << std::endl;
        std::cout << "sm2     : " << currentLoad.sm2 << std::endl;
        std::cout << "nist256 : " << currentLoad.nist256 << std::endl;
        std::cout << "nist384 : " << currentLoad.nist384 << std::endl;
        std::cout << "bp256   : " << currentLoad.bp256 << std::endl;
        std::cout << "bp384   : " << currentLoad.bp384 << std::endl;
    }
};

int main(int argc, char **argv) {

    telux::common::ErrorCode ec;
    std::shared_ptr<telux::sec::ICAControlManager> caCtrlMgr;
    std::shared_ptr<StatsListener> statsListener;
    telux::sec::LoadConfig loadConfig{};

    /* Step - 1 */
    auto &secFact = telux::sec::SecurityFactory::getInstance();

    /* Step - 2 */
    caCtrlMgr = secFact.getCAControlManager(ec);
    if (!caCtrlMgr) {
        std::cout <<
         "can't get ICAControlManager, err " << static_cast<int>(ec) << std::endl;
        return -ENOMEM;
    }

    try {
        statsListener = std::make_shared<StatsListener>();
    } catch (const std::exception& e) {
        std::cout << "can't create StatsListener" << std::endl;
        return -ENOMEM;
    }

    /* Step - 4 */
    ec = caCtrlMgr->registerListener(statsListener);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't register, " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    /* Step - 5 */
    loadConfig.calculationInterval = 100; /* 100 milliseconds */
    ec = caCtrlMgr->startMonitoring(loadConfig);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't start monitoring, " << static_cast<int>(ec) << std::endl;
        caCtrlMgr->deRegisterListener(statsListener);
        return -EIO;
    }

    /* Let load become available, listener invoked, before we exit the application */
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    /* Step - 6 */
    ec = caCtrlMgr->stopMonitoring();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't stop monitoring, " << static_cast<int>(ec) << std::endl;
        caCtrlMgr->deRegisterListener(statsListener);
        return -EIO;
    }

    /* Step - 7 */
    caCtrlMgr->deRegisterListener(statsListener);

    return 0;
}

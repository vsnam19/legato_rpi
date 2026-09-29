/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to register a client to receive QMI transaction
 * details that caused the system wakeup. The steps are as follows:
 *
 * 1. Get a PowerFactory instance.
 * 2. Get a IWakeupManager instance from the PowerFactory.
 * 3. Wait for the power service to become available.
 * 4. Register for wake up reason details.
 * 5. When wake up reason details are received, execute application specific logic.
 * 6. Finally, when the use case is over, deregister the listener.
 *
 * Usage:
 * # ./qmi_wakeup_listener <wait-duration>
 *
 * This will wait for <wait-duration> seconds to receive qmi details.
 */

#include <errno.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <cstdio>
#include <future>

#include <telux/common/CommonDefines.hpp>
#include <telux/power/PowerFactory.hpp>
#include <telux/power/WakeupManager.hpp>

class WakeupReasonListener : public telux::power::IWakeupListener {
 public:
    /* Step - 5 */
    void onWakeup(telux::power::WakeupInfo wakeupInfo) override {

        std::printf("onWakeup()\n");
        if (wakeupInfo.wakeupType != telux::power::WakeupType::QMI) {
            return;
        }

        std::printf("serviceId : %u\n", wakeupInfo.qmiWakeupInfo.serviceId);
        std::printf("sourceNodeId : %u\n", wakeupInfo.qmiWakeupInfo.sourceNodeId);
        std::printf("destinationNodeId : %u\n", wakeupInfo.qmiWakeupInfo.destinationNodeId);

        if (wakeupInfo.qmiWakeupInfo.isMsgIdValid) {
            std::printf("msgId : %u\n", wakeupInfo.qmiWakeupInfo.msgId);
        }
        if (wakeupInfo.qmiWakeupInfo.isPIDValid) {
            std::printf("pid : %u\n", wakeupInfo.qmiWakeupInfo.pid);
        }
        if (wakeupInfo.qmiWakeupInfo.isProcessNameValid) {
            std::printf("processName : %s\n", wakeupInfo.qmiWakeupInfo.processName.c_str());
        }
    }
};

class Application {
 public:
    int init() {
        telux::common::ErrorCode ec;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &powerFactory = telux::power::PowerFactory::getInstance();

        /* Step - 2 */
        wakeupMgr_ = powerFactory.getWakeupManager(
            [&p](telux::common::ServiceStatus srvStatus) {
            p.set_value(srvStatus);
        });

        if (!wakeupMgr_) {
            std::cout << "Can't get IWakeupManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Power service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        /* Step - 4 */
        try {
            wakeupReasonListener_ = std::make_shared<WakeupReasonListener>();
        } catch (const std::exception& e) {
            std::cout << "Can't allocate WakeupReasonListener" << std::endl;
            return -ENOMEM;
        }

        ec = wakeupMgr_->registerListener(wakeupReasonListener_);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't register listener, err " <<
                static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete, listener registered" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::ErrorCode ec;

        /* Step - 6 */
        ec = wakeupMgr_->deRegisterListener(wakeupReasonListener_);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        std::cout << "Listener deregistered" << std::endl;
        return 0;
    }

 private:
    std::shared_ptr<WakeupReasonListener> wakeupReasonListener_;
    std::shared_ptr<telux::power::IWakeupManager> wakeupMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<Application> app;

    if (argc < 2) {
        std::cout << "Usage: qmi_wakeup_listener <wait-duration>" << std::endl;
        return -EINVAL;
    }

    try {
        app = std::make_shared<Application>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate Application" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    /* Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(std::stoul(argv[1])));

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}

/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2021, 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 * This application demonstrates how to set certain cells as dubious for LTE/NR.
 * The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a INetworkSelectionManager instance from the PhoneFactory.
 * 3. Wait for the network selection manager service to become available.
 * 4. Set LTE cells to dubious.
 * 5. Set NR cells to dubious.
 * 7. Deinit app.
 *
 * Usage:
 * # ./smart_network_selection_app <SlotId (1 / 2)>
 * E.g. ./smart_network_selection_app 1
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/ServingSystemDefines.hpp>
#include <telux/tel/NetworkSelectionManager.hpp>

class SmartNetworkSelectionApp : public telux::tel::INetworkSelectionListener,
                                 public std::enable_shared_from_this<SmartNetworkSelectionApp> {
 public:
    int init(int slotId) {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        nwSelectionMgr_ = phoneFactory.getNetworkSelectionManager(slotId,
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!nwSelectionMgr_) {
            std::cout << "Can't get INetworkSelectionManager" << std::endl;
            return -ENOMEM;
        }

        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Network selection manager service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        auto status = nwSelectionMgr_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int userInputForLteCell(std::vector<telux::tel::LteDubiousCell> &lteDbCellInfoList) {
        telux::tel::LteDubiousCell lteDbCellInfo;
        lteDbCellInfo.ci.mcc   = "10";
        lteDbCellInfo.ci.mnc   = "11";
        lteDbCellInfo.ci.arfcn = 2;
        lteDbCellInfo.ci.pci   = 10;
        lteDbCellInfo.ci.activeBand    = telux::tel::RFBand::E_UTRA_OPERATING_BAND_1;
        lteDbCellInfo.ci.causeCodeMask = std::bitset<32>(
                telux::tel::DubiousCellCauseCode::DUBIOUS_CELL_CAUSE_CEF);
        lteDbCellInfo.cgi = 25;
        lteDbCellInfoList.push_back(lteDbCellInfo);
        return 0;
    }

    int userInputForNrCell(std::vector<telux::tel::NrDubiousCell> &nrDbCellInfoList) {
        telux::tel::NrDubiousCell nrDbCellInfo;
        nrDbCellInfo.ci.mcc   = "11";
        nrDbCellInfo.ci.mnc   = "12";
        nrDbCellInfo.ci.arfcn = 422001;
        nrDbCellInfo.ci.pci   = 10;
        nrDbCellInfo.ci.activeBand    = telux::tel::RFBand::NR5G_BAND_1;
        nrDbCellInfo.ci.causeCodeMask = std::bitset<32>(
                telux::tel::DubiousCellCauseCode::DUBIOUS_CELL_CAUSE_RLF);
        nrDbCellInfo.cgi = 26;
        nrDbCellInfo.spacing = telux::tel::NrSubcarrierSpacing::SCS_15;
        nrDbCellInfoList.push_back(nrDbCellInfo);
        return 0;
    }

    int setLteDubiousCell(const std::vector<telux::tel::LteDubiousCell> &params) {
        auto errCode = nwSelectionMgr_->setLteDubiousCell(params);
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't set LTE dubious cell params, err " <<
                static_cast<int>(errCode) << std::endl;
            return -EIO;
        }

        std::cout << " set LTE dubious cell params succeed" << std::endl;
        return 0;
    }

    int setNrDubiousCell(const std::vector<telux::tel::NrDubiousCell> &params) {
        auto errCode = nwSelectionMgr_->setNrDubiousCell(params);
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't set NR dubious cell params, err " <<
                static_cast<int>(errCode) << std::endl;
            return -EIO;
        }

        std::cout << " set NR dubious cell params succeed" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::Status status;

        status = nwSelectionMgr_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " <<
                static_cast<int>(status) << std::endl;
            return -EIO;
        }
        return 0;
    }

 private:
    std::shared_ptr<telux::tel::INetworkSelectionManager> nwSelectionMgr_;
};

int main(int argc, char *argv[]) {

    int ret,slotId;
    std::shared_ptr<SmartNetworkSelectionApp> app;
    std::vector<telux::tel::LteDubiousCell> lteDbCellInfoList;
    std::vector<telux::tel::NrDubiousCell> nrDbCellInfoList;

    if (argc != 2) {
        std::cout << "./smart_network_selection_app <SlotId>" << std::endl;
        return -EINVAL;
    }

    if ((std::atoi(argv[1]) != SlotId::SLOT_ID_1) || (std::atoi(argv[1]) != SlotId::SLOT_ID_2)) {
        std::cout << " Invalid slotId, valid values: 1/2" << std::endl;
        return -EINVAL;
    }
    slotId = static_cast<int>(std::atoi(argv[1]));

    try {
        app = std::make_shared<SmartNetworkSelectionApp>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate: insufficient memory" << std::endl;
        return -ENOMEM;
    }

    /** Step - 1 */
    ret = app->init(slotId);
    if (ret < 0) {
        return ret;
    }

    /** Step - 2 */
    ret = app->userInputForLteCell(lteDbCellInfoList);
    if (ret < 0) {
        return ret;
    }

    /** Step - 3 */
    ret = app->setLteDubiousCell(lteDbCellInfoList);
    if (ret < 0) {
        return ret;
    }

    /** Step - 4 */
    ret = app->userInputForNrCell(nrDbCellInfoList);
    if (ret < 0) {
        return ret;
    }

    /** Step - 5 */
    ret = app->setNrDubiousCell(nrDbCellInfoList);
    if (ret < 0) {
        return ret;
    }

    /** Step - 6 */
    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nSmart network selection app exiting" << std::endl;
    return 0;
}

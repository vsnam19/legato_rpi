/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef WLANSTAINTERFACEMANAGERMENU_HPP
#define WLANSTAINTERFACEMANAGERMENU_HPP

#include <iostream>
#include <memory>

#include <telux/wlan/WlanFactory.hpp>
#include <telux/wlan/StaInterfaceManager.hpp>

#include "console_app_framework/ConsoleApp.hpp"
#include "WlanUtils.hpp"
#include "../../common/utils/Utils.hpp"

class WlanStaInterfaceManagerMenu : public ConsoleApp ,
                                    public telux::wlan::IStaListener,
                                    public std::enable_shared_from_this<
                                       WlanStaInterfaceManagerMenu> {
 public:
    WlanStaInterfaceManagerMenu(std::string appName, std::string cursor);
    ~WlanStaInterfaceManagerMenu();

    /**
     * Initialize commands
     */
    bool init();
    void showMenu();

    void setIpConfig(std::vector<std::string> userInput);
    void getConfig(std::vector<std::string> userInput);
    void getStatus(std::vector<std::string> userInput);
    void enableHotspot2(std::vector<std::string> userInput);
    void setBridgeMode(std::vector<std::string> userInput);
    void startScan(std::vector<std::string> userInput);
    void addNetworkConfig(std::vector<std::string> userInput);
    void removeNetworkConfig(std::vector<std::string> userInput);
    void getNetworkConfigs(std::vector<std::string> userInput);
    void connect(std::vector<std::string> userInput);
    void disconnect(std::vector<std::string> userInput);
    void manageStaService(std::vector<std::string> userInput);
    void onStationStatusChanged(std::vector<telux::wlan::StaStatus> status) override;
    void onScanResultUpdated(const telux::wlan::StaScanResult &staScanResult) override;
    void onStationBandChanged(telux::wlan::BandType radio) override;
 private:
    bool menuOptionsAdded_;
    std::shared_ptr<telux::wlan::IStaInterfaceManager> wlanStaInterfaceManager_ = nullptr;

};
#endif

/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to configure and use WLAN STA operations.
 * The steps are as follows:
 *
 * 1. Get a WlanFactory instance.
 * 2. Get a IWlanDeviceManager and IStaInterfaceManager instance from the WlanFactory.
 * 3. Wait for the WLAN service to become available.
 * 4. Register listeners that will receive WLAN state change updates.
 * 5. Disable WLAN, if it is enabled currently.
 * 6. Update WLAN configuration by specifying number of the
 *    access points and number of the stations.
 * 7. Enable the WLAN for the configuration to take effect.
 * 8. Initiate STA scan process.
 * 9. Add the network configuration once the scan is complete.
 * 10. Expect indication on connection status.
 * 11. Deregister the listener.
 *
 * Usage:
 * # wlan_sta_config_app --ssid <SSID> --passphrase <Passphrase> [--priority <Priority>]
 *            [--band <Band>] [--bssid <BSSID>]
 *
 * Example - ./wlan_sta_config_app test testpasswd 99 0 88:88:88:88:88:88
 *
 * for Band configuration refer to mapping below
 * <Band>  1-2.4GHz, 2-5 GHz, 3-6GHz, default- 0 (UNSPECIFIED)
 *
 * Please note that priority, band and bssid are optional params here.
 */

#include <errno.h>

#include <iostream>
#include <iomanip>
#include <memory>
#include <cstdlib>
#include <future>
#include <thread>
#include <unordered_map>

#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanFactory.hpp>
#include <telux/wlan/WlanDeviceManager.hpp>
#include <telux/wlan/StaInterfaceManager.hpp>

#define APCOUNT 0
#define STACOUNT 1

class WlanStaConfigurator : public telux::wlan::IWlanListener,
                            public telux::wlan::IStaListener,
                            public std::enable_shared_from_this<WlanStaConfigurator> {
 public:
    int init() {
        telux::common::ErrorCode ec;
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &wlanFactory = telux::wlan::WlanFactory::getInstance();

        /* Step - 2 */
        wlanDevMgr_ = wlanFactory.getWlanDeviceManager(
                [&p](telux::common::ServiceStatus status) {
            p.set_value(status);
        });

        if (!wlanDevMgr_) {
            std::cout << "Can't get IWlanDeviceManager" << std::endl;
            return -ENOMEM;
        }

        wlanStaMgr_ = wlanFactory.getStaInterfaceManager();

        if (!wlanStaMgr_) {
            std::cout << "Can't get IStaInterfaceManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "WLAN service unavailable, status " <<
                static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        /* Step - 4 */
        ec = wlanDevMgr_->registerListener(shared_from_this());
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't register to IWlanListener" << std::endl;
            return -EIO;
        }

        ec = wlanStaMgr_->registerListener(shared_from_this());
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't register to IStaListener" << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::ErrorCode ec;

        /* Step - 11 */
        ec = wlanDevMgr_->deregisterListener(shared_from_this());
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Could't deregister from IWlanListener, err " <<
                static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        ec = wlanStaMgr_->deregisterListener(shared_from_this());
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Could't deregister from IStaListener, err " <<
                static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int disableWLAN() {
        bool isEnabled = false;
        telux::common::ErrorCode ec;
        std::vector<telux::wlan::InterfaceStatus> ifaceStatus;

        ec = wlanDevMgr_->getStatus(isEnabled, ifaceStatus);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't get current state, err " <<
                static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        if (isEnabled) {
            enablePromise_ = std::promise<bool>();

            /* Step - 5 */
            ec = wlanDevMgr_->enable(false);
            if (ec != telux::common::ErrorCode::SUCCESS) {
                std::cout << "Can't disable WLAN, err " <<
                    static_cast<int>(ec) << std::endl;
                return -EIO;
            }

            isEnabled = enablePromise_.get_future().get();
            if (isEnabled) {
                std::cout << "Failed to disable WLAN" << std::endl;
                return -EIO;
            }
        }

        std::cout << "WLAN disabled" << std::endl;
        return 0;
    }

    int updateConfiguration(int APCount, int STACount) {
        telux::common::ErrorCode ec;

        /* Step - 6 */
        ec = wlanDevMgr_->setMode(APCount, STACount);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't set config, err " << static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        std::cout << "\nMode set successfully" << std::endl;
        return 0;
    }

    int enableWLAN() {
        bool isEnabled = false;
        telux::common::ErrorCode ec;

        enablePromise_ = std::promise<bool>();

        /* Step - 7 */
        ec = wlanDevMgr_->enable(true);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't enable WLAN, err " << static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        isEnabled = enablePromise_.get_future().get();
        if (!isEnabled) {
            std::cout << "Failed to enable WLAN" << std::endl;
            return -EIO;
        }

        std::cout << "WLAN enabled" << std::endl;
        return 0;
    }

    int startScan() {
        telux::common::ErrorCode ec;
        /* Step - 8 */
        ec = wlanStaMgr_->startScan(telux::wlan::Id::PRIMARY);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't enable WLAN Scan , err " << static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        std::cout << "\nWLAN STA Scan enabled." << std::endl;
        return 0;
    }

    int addNetworkConfig(const telux::wlan::StaNetworkConfigEntry& staNetworkConfigEntry) {

        telux::common::ErrorCode ec;
        scanCompletePromise_ = std::promise<bool>();

        /* Step - 9 */
        bool scanCompleted = scanCompletePromise_.get_future().get();
        if(scanCompleted) {
            std::cout << "Adding Network Config" << std::endl;
            ec = wlanStaMgr_->addNetworkConfig(telux::wlan::Id::PRIMARY, staNetworkConfigEntry);
            if (ec != telux::common::ErrorCode::SUCCESS) {
                std::cout << "Can't add NetworkConfig entry, err " << static_cast<int>(ec)
                          << std::endl;
                return -EIO;
            }
        }

        std::cout << "Adding Network Config was successful" << std::endl;
        return 0;
    }

    void onEnableChanged(bool enable) {
        std::cout << "\nonEnableChanged()" << std::endl;
        std::cout << "New value: " << enable << std::endl;
        enablePromise_.set_value(enable);
    }

    void onScanResultUpdated(const telux::wlan::StaScanResult &staScanResult) {
        std::cout << "--------------------------------------------" << std::endl;
        std::cout << "Id                                  : "
                  << getWlanId(staScanResult.staId) << std::endl;
        std::cout << "Batch index                         : "
                  << staScanResult.batchIndex << std::endl;
        std::cout << "is Scan Complete? : "
                  << ((staScanResult.isScanComplete)? "Yes":"No") << std::endl;
        if(staScanResult.externalApList.size() > 0) {
            std::cout << std::left << std::setw(18) << "\nBSSID "
            << std::setw(10) << " | Frequency "
            << std::setw(10) << " | Signal Level "
            << std::setw(23) << " | Flags "
            << " | SSID\n" << std::endl;

            for(auto& externalAp:staScanResult.externalApList) {
                std::cout << std::left << std::setw(20) << externalAp.bssid
                << std::setw(10) << RadioTypeToString(externalAp.band)
                << std::setw(10) << externalAp.signalStrength
                << std::setw(30) << externalAp.securityFlags
                << externalAp.ssid << std::endl;
            }
            std::cout << std::endl;
        } else {
            std::cout << "No External APs were found" << std::endl;
        }
        if(staScanResult.isScanComplete) {
            scanCompletePromise_.set_value(true);
        }
    }

    void onStationStatusChanged(std::vector<telux::wlan::StaStatus> status) {
        /* Step - 10 */
        if(status.size() > 0) {
            std::cout << "List of Stations:" << std::endl;
            for(auto& sta:status) {
                std::cout << "--------------------------------------------" << std::endl;
                std::cout << "Id                : " << getWlanId(sta.id) << std::endl;
                std::cout << "Network Interface : " << sta.name << std::endl;
                std::cout << "IPv4 Addr         : " << sta.ipv4Address << std::endl;
                std::cout << "IPv6 Addr         : " << sta.ipv6Address << std::endl;
                std::cout << "MAC Addr          : " << sta.macAddress  << std::endl;
                std::cout << "Interface Status  : "
                            << getStaInterfaceStatus(sta.status) << std::endl;
                if(sta.status == telux::wlan::StaInterfaceStatus::ASSOCIATION_FAILED) {
                    std::cout << "Connection status : "
                                << getStaConnectionStatus(sta.connectionStatus) << std::endl;
                }
            }
            std::cout << std::endl;
        } else {
            std::cout << "No Station is currently active" << std::endl;
        }
    }

 private:
    std::promise<bool> enablePromise_;
    std::promise<bool> scanCompletePromise_;
    std::shared_ptr<telux::wlan::IWlanDeviceManager> wlanDevMgr_;
    std::shared_ptr<telux::wlan::IStaInterfaceManager> wlanStaMgr_;

    std::string getWlanId(telux::wlan::Id id) {
        std::string retStr;
        switch(id) {
           case telux::wlan::Id::PRIMARY:
              retStr = "PRIMARY";
              break;
           case telux::wlan::Id::SECONDARY:
              retStr = "SECONDARY";
              break;
           case telux::wlan::Id::TERTIARY:
              retStr = "TERTIARY";
              break;
           case telux::wlan::Id::QUATERNARY:
              retStr = "QUATERNARY";
              break;
        }
        return retStr;
     }

    std::string getStaInterfaceStatus(telux::wlan::StaInterfaceStatus status) {
        std::string retStr = "";
        switch(status) {
           case telux::wlan::StaInterfaceStatus::UNKNOWN:
              retStr = "UNKNOWN";
              break;
           case telux::wlan::StaInterfaceStatus::CONNECTING:
              retStr = "CONNECTING";
              break;
           case telux::wlan::StaInterfaceStatus::CONNECTED:
              retStr = "CONNECTED";
              break;
           case telux::wlan::StaInterfaceStatus::DISCONNECTED:
              retStr = "DISCONNECTED";
              break;
           case telux::wlan::StaInterfaceStatus::ASSOCIATION_FAILED:
              retStr = "ASSOCIATION_FAILED";
              break;
           case telux::wlan::StaInterfaceStatus::IP_ASSIGNMENT_FAILED:
              retStr = "IP_ASSIGNMENT_FAILED";
              break;
           default:
              break;
        }
        return retStr;
     }

     std::string getStaConnectionStatus(telux::wlan::StaConnectionStatus status) {
        std::string retStr = "";
        switch(status) {
           case telux::wlan::StaConnectionStatus::UNKNOWN:
              retStr = "UNKNOWN";
              break;
           case telux::wlan::StaConnectionStatus::SUCCESS:
              retStr = "SUCCESS";
              break;
           case telux::wlan::StaConnectionStatus::INCORRECT_PSK:
              retStr = "INCORRECT_PSK";
              break;
           case telux::wlan::StaConnectionStatus::AP_NOT_FOUND:
              retStr = "AP_NOT_FOUND";
              break;
           default:
              break;
        }
        return retStr;
     }

    std::string RadioTypeToString(telux::wlan::BandType radio) {
        std::string retString = "";
        switch(radio) {
           case telux::wlan::BandType::BAND_5GHZ:
              retString = "5 GHZ";
              break;
           case telux::wlan::BandType::BAND_2GHZ:
              retString = "2.4 GHZ";
              break;
           case telux::wlan::BandType::BAND_6GHZ:
              retString = "6 GHZ";
              break;
           default:
              break;
        }
        return retString;
     }
};

void parseArguments(int argc, char* argv[], std::unordered_map<std::string, std::string>& args) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (i + 1 < argc) {
            args[arg] = argv[++i];
        }
    }
}

void printUsage() {
    std::cout << "Usage: wlan_sta_config_app --ssid <SSID> --passphrase <Passphrase> \
                  [--priority <Priority>] [--band <Band>] [--bssid <BSSID>]\n";
    std::cout << "Mandatory parameters:\n";
    std::cout << "  --ssid       : SSID of the Wi-Fi network\n";
    std::cout << "  --passphrase : Passphrase for the Wi-Fi network\n";
    std::cout << "Optional parameters:\n";
    std::cout << "  --priority   : Priority of the network (default: 0)\n";
    std::cout << "  --band       : Band of the network (default: 0)\n";
    std::cout << "  --bssid      : BSSID of the network (default: empty string)\n";
}

int main(int argc, char *argv[]) {

    std::shared_ptr<WlanStaConfigurator> app;
    std::unordered_map<std::string, std::string> args;
    parseArguments(argc, argv, args);

    std::string ssid = args.count("--ssid") ? args.at("--ssid") : "";
    std::string passphrase = args.count("--passphrase") ? args.at("--passphrase") : "";
    int priority = args.count("--priority") ? std::stoi(args.at("--priority")) : 0;
    int band = args.count("--band") ? std::stoi(args.at("--band")) : 1;
    std::string bssid = args.count("--bssid") ? args.at("--bssid") : " ";

    if (ssid.empty() || passphrase.empty()) {
        printUsage();
        return -EINVAL;
    }

    try {
        app = std::make_shared<WlanStaConfigurator>();
    } catch (const std::exception& e) {
        std::cout << "Can't allocate WlanStaConfigurator" << std::endl;
        return -ENOMEM;
    }

    int ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->disableWLAN();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->updateConfiguration(APCOUNT, STACOUNT);
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->enableWLAN();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    ret = app->startScan();
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    telux::wlan::StaNetworkConfigEntry staNetworkConfigEntry = {};
    staNetworkConfigEntry.ssid       = ssid;
    staNetworkConfigEntry.passPhrase = passphrase;
    if(band != 0) {
        if(band == 1) {
            staNetworkConfigEntry.band = telux::wlan::BandType::BAND_2GHZ;
        } else if (band == 2) {
            staNetworkConfigEntry.band = telux::wlan::BandType::BAND_5GHZ;
        } else {
            staNetworkConfigEntry.band = telux::wlan::BandType::BAND_6GHZ;
        }
    }
    staNetworkConfigEntry.priority = priority;
    staNetworkConfigEntry.bssid = bssid;

    ret = app->addNetworkConfig(staNetworkConfigEntry);
    if (ret < 0) {
        app->deinit();
        return ret;
    }

    /* Wait for receiving all asynchronous responses before exiting the
    * application. Application specific logic goes here, this wait is just an
    * example */
    std::this_thread::sleep_for(std::chrono::seconds(25));

    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nWlan station config app exiting" << std::endl;
    return 0;
}

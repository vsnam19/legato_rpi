/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "WlanStaInterfaceManagerMenu.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

WlanStaInterfaceManagerMenu::WlanStaInterfaceManagerMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
       menuOptionsAdded_ = false;
}

WlanStaInterfaceManagerMenu::~WlanStaInterfaceManagerMenu() {
    if (wlanStaInterfaceManager_) {
        wlanStaInterfaceManager_ = nullptr;
    }
}

bool WlanStaInterfaceManagerMenu::init() {
    if (wlanStaInterfaceManager_ == nullptr) {
        auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
        wlanStaInterfaceManager_ = wlanFactory.getStaInterfaceManager();
        if (wlanStaInterfaceManager_ == nullptr) {
            //Return immediately
            std::cout <<
                "\nError encountered in initializing Wlan Station Interface Manager" << std::endl;
            return false;
        }
        wlanStaInterfaceManager_->registerListener(shared_from_this());
    }
    return true;
}

void WlanStaInterfaceManagerMenu::showMenu() {
    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
		  int stepID = 1;
        std::shared_ptr<ConsoleAppCommand> setIpConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "set_ip_config", {},
            std::bind(&WlanStaInterfaceManagerMenu::setIpConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setBridgeMode
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "set_bridge_mode", {},
            std::bind(&WlanStaInterfaceManagerMenu::setBridgeMode, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> enableHotspot2
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "enable_hotspot2_support", {},
            std::bind(&WlanStaInterfaceManagerMenu::enableHotspot2, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "get_config", {},
            std::bind(&WlanStaInterfaceManagerMenu::getConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getStatus
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "get_status", {},
            std::bind(&WlanStaInterfaceManagerMenu::getStatus, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> startScan
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "start_scan", {},
            std::bind(&WlanStaInterfaceManagerMenu::startScan, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> addNetworkConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "add_network_config", {},
            std::bind(&WlanStaInterfaceManagerMenu::addNetworkConfig, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> removeNetworkConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "remove_network_config", {},
            std::bind(&WlanStaInterfaceManagerMenu::removeNetworkConfig, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getNetworkConfigs
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "get_network_configs", {},
            std::bind(&WlanStaInterfaceManagerMenu::getNetworkConfigs, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> connect
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "connect", {},
            std::bind(&WlanStaInterfaceManagerMenu::connect, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> disconnect
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "disconnect", {},
            std::bind(&WlanStaInterfaceManagerMenu::disconnect, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> manageStaService
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "manage_service", {},
            std::bind(&WlanStaInterfaceManagerMenu::manageStaService, this,
                std::placeholders::_1)));
        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {
            setIpConfig, setBridgeMode, enableHotspot2, getConfig, getStatus, startScan,
            addNetworkConfig, removeNetworkConfig, getNetworkConfigs, connect, disconnect,
            manageStaService};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
}

void WlanStaInterfaceManagerMenu::setIpConfig(std::vector<std::string> userInput) {

    int staId = 1, ipConfig = 1;
    telux::wlan::StaIpConfig staIpConfig = {};
    telux::wlan::StaStaticIpConfig staticIpConfig;

    std::cout << "Set Station IP Configuration" << std::endl;

    std::cout << "Select Station IP Type (1-Dynamic IP, 2-Static IP): ";
    std::cin >> ipConfig;
    WlanUtils::validateInput(ipConfig, {1, 2});

    if(ipConfig == 2){
        staIpConfig = telux::wlan::StaIpConfig::STATIC_IP;
        std::string userInput{};
        std::cout << "Enter IPv4 Address: ";
        std::cin >> userInput;
        Utils::validateInput(userInput);
        staticIpConfig.ipAddr = userInput;
        std::cout << std::endl;

        std::cout << "Enter Gateway IPv4 Address: ";
        std::cin >> userInput;
        Utils::validateInput(userInput);
        staticIpConfig.gwIpAddr = userInput;
        std::cout << std::endl;

        std::cout << "Enter Subnet Mask: ";
        std::cin >> userInput;
        Utils::validateInput(userInput);
        staticIpConfig.netMask = userInput;
        std::cout << std::endl;

        std::cout << "Enter DNS IPv4 Address: ";
        std::cin >> userInput;
        Utils::validateInput(userInput);
        staticIpConfig.dnsAddr = userInput;
        std::cout << std::endl;
    }else{
        staIpConfig = telux::wlan::StaIpConfig::DYNAMIC_IP;
    }
    telux::common::ErrorCode retCode = wlanStaInterfaceManager_->setIpConfig(
        static_cast<telux::wlan::Id>(staId), staIpConfig, staticIpConfig);

    std::cout << "\nSet Station IP Configuration Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanStaInterfaceManagerMenu::setBridgeMode(std::vector<std::string> userInput) {

    std::cout << "Set Station Bridge Mode" << std::endl;

    int staId = 1, bridgeMode = 0;
    std::shared_ptr<telux::wlan::StaStaticIpConfig> staticIpConfig;

    std::cout << "Enter Bridge Mode (0-Router Mode, 1-Bridge Mode): ";
    std::cin >> bridgeMode;
    WlanUtils::validateInput(bridgeMode, {0, 1});

    telux::common::ErrorCode retCode = wlanStaInterfaceManager_->setBridgeMode(
        static_cast<telux::wlan::Id>(staId), static_cast<telux::wlan::StaBridgeMode>(bridgeMode));

    std::cout << "\nSet Bridge Mode Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanStaInterfaceManagerMenu::enableHotspot2(std::vector<std::string> userInput) {

    std::cout << "Enable Support For Hotspot 2.0" << std::endl;

    int hotspotEnable;

    std::cout << "Enable/Disable Hotspot 2.0 Support (1-enable, 0-disable): ";
    std::cin >> hotspotEnable;
    std::cout << std::endl;
    WlanUtils::validateInput(hotspotEnable, {0, 1});

    telux::common::ErrorCode retCode =
        wlanStaInterfaceManager_->enableHotspot2(
            telux::wlan::Id::PRIMARY, static_cast<bool>(hotspotEnable));
    std::cout << "\nEnable Hotspot2 Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}


void WlanStaInterfaceManagerMenu::getConfig(std::vector<std::string> userInput) {
    std::vector<telux::wlan::StaConfig> config;

    std::cout << "Request Station Configuration" << std::endl;
    telux::common::ErrorCode retCode = wlanStaInterfaceManager_->getConfig(config);

    std::cout << "\nrequest Station Configuration Response"
                << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(retCode)
                << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        for (auto &cfg : config) {
            std::cout << "------------------------------------------" << std::endl;
            std::cout << "Id         : " << WlanUtils::getWlanId(cfg.staId) << std::endl;
            std::cout << "IP config  : "
                      << ((cfg.ipConfig == telux::wlan::StaIpConfig::DYNAMIC_IP)?
                            "DYNAMIC":"STATIC") << std::endl;
            if(cfg.ipConfig == telux::wlan::StaIpConfig::STATIC_IP) {
                std::cout << "IPv4 Addr        : " << cfg.staticIpConfig.ipAddr << std::endl;
                std::cout << "Gateway IPv4 Addr: " << cfg.staticIpConfig.gwIpAddr << std::endl;
                std::cout << "Subnet Mask      : " << cfg.staticIpConfig.netMask << std::endl;
                std::cout << "DNS IPv4 Addr    : " << cfg.staticIpConfig.dnsAddr << std::endl;
            }
            std::cout << "Bridge Mode: "
                        << ((cfg.bridgeMode == telux::wlan::StaBridgeMode::BRIDGE)?
                            "Bridge":"Router") << std::endl;
        }
    }
}

void WlanStaInterfaceManagerMenu::getStatus(std::vector<std::string> userInput) {
    std::vector<telux::wlan::StaStatus> status;
    std::cout << "Request Station Status" << std::endl;

    telux::common::ErrorCode retCode = wlanStaInterfaceManager_->getStatus(status);
    std::cout << "\nRequest Station Status Response"
                << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(retCode)
                << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        WlanUtils::printStaStatus(status);
    }
}


void WlanStaInterfaceManagerMenu::startScan(std::vector<std::string> userInput) {
    std::cout << "Start scan for nearby APs" << std::endl;

    telux::common::ErrorCode retCode =
        wlanStaInterfaceManager_->startScan(telux::wlan::Id::PRIMARY);

    std::cout << "\nStart scan Response"
            << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
            << ". ErrorCode: " << static_cast<int>(retCode)
            << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;

}

void WlanStaInterfaceManagerMenu::addNetworkConfig(std::vector<std::string> userInput) {
    std::cout << "Add Network Config entry" << std::endl;

    telux::wlan::StaNetworkConfigEntry staNetConfigEntry = {};
    std::string ssid = "";
    std::cout << "Enter SSID (Without Quotes): ";
    std::cin >> ssid;
    Utils::validateInput(ssid);
    staNetConfigEntry.ssid = ssid;

    int input = 0;
    std::cout << "Is WLAN accesspoint password protected? (0-NO, 1-YES): ";
    std::cin >> input;
    std::cout << std::endl;
    WlanUtils::validateInput(input, {0, 1});
    if(input) {
        std::string passPhrase = "";
        std::cout << "Enter passphrase (Without Quotes): ";
        std::cin >> passPhrase;
        Utils::validateInput(passPhrase);
        staNetConfigEntry.passPhrase = passPhrase;
    }


    int userResp = 0;
    std::cout << "Enter STA operational band type \
                    (1- 2.4GHz, 2- 5 GHz, 3- 6GHz): ";
    std::cin >> userResp;
    WlanUtils::validateInput(userResp, {1, 2, 3});
    std::cout << std::endl;
    telux::wlan::ApNetConfig apNetConfig = {};
    if(userResp == 1) {
        staNetConfigEntry.band = telux::wlan::BandType::BAND_2GHZ;
    } else if (userResp == 2) {
        staNetConfigEntry.band = telux::wlan::BandType::BAND_5GHZ;
    } else {
        staNetConfigEntry.band = telux::wlan::BandType::BAND_6GHZ;
    }

    char delimiter = '\n';
    std::string priority;
    std::cout << "Enter priority (optional) : ";
    std::getline(std::cin, priority, delimiter);

    if (!priority.empty()) {
        try {
            staNetConfigEntry.priority = std::stoi(priority);
        } catch (const std::exception &e) {
            std::cout << "ERROR: "<< e.what() << std::endl;
            return;
        }
    }

    std::cout << "Enter BSSID (optional) : ";
    std::getline(std::cin, staNetConfigEntry.bssid, delimiter);

    int enable;
    std::cout << "Enable the configured network?  (1-YES, 0-NO): ";
    std::cin >> enable;
    std::cout << std::endl;
    WlanUtils::validateInput(enable, {0, 1});
    staNetConfigEntry.enable = static_cast<bool>(enable);

    telux::common::ErrorCode retCode = wlanStaInterfaceManager_->addNetworkConfig(
        telux::wlan::Id::PRIMARY, staNetConfigEntry);
    std::cout << "\nAdd Network config Response"
            << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
            << ". ErrorCode: " << static_cast<int>(retCode)
            << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;

}

void WlanStaInterfaceManagerMenu::removeNetworkConfig(std::vector<std::string> userInput){
    std::cout << "Remove network config entry" << std::endl;

    telux::wlan::NetworkId networkId;
    std::cout << "Enter NetworkId : ";
    std::cin >> networkId;
    Utils::validateInput(networkId);

    telux::common::ErrorCode retCode =
        wlanStaInterfaceManager_->removeNetworkConfig(telux::wlan::Id::PRIMARY, networkId);

    std::cout << "\nRemove network config Response"
            << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
            << ". ErrorCode: " << static_cast<int>(retCode)
            << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanStaInterfaceManagerMenu::getNetworkConfigs(std::vector<std::string> userInput) {
    std::cout << "Get Saved network block entries." << std::endl;

    std::vector<telux::wlan::StaNetworkConfigInfo> networkConfigInfo;

    telux::common::ErrorCode retCode =
        wlanStaInterfaceManager_->getNetworkConfigs(telux::wlan::Id::PRIMARY, networkConfigInfo);

    std::cout << "\nGet network configs Response"
            << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
            << ". ErrorCode: " << static_cast<int>(retCode)
            << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        WlanUtils::printNetworkConfigs(networkConfigInfo);
    }
}

void WlanStaInterfaceManagerMenu::connect(std::vector<std::string> userInput) {
    std::cout << "Connect to External AP" << std::endl;

    telux::wlan::NetworkId networkId;
    std::cout << "Enter NetworkId : ";
    std::cin >> networkId;
    Utils::validateInput(networkId);

    telux::common::ErrorCode retCode =
        wlanStaInterfaceManager_->connect(telux::wlan::Id::PRIMARY, networkId);

    std::cout << "\nConnecting to External AP with NetworkId: " << networkId
            << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
            << ". ErrorCode: " << static_cast<int>(retCode)
            << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanStaInterfaceManagerMenu::disconnect(std::vector<std::string> userInput) {
    std::cout << "Disconnect from External AP" << std::endl;

    telux::common::ErrorCode retCode =
        wlanStaInterfaceManager_->disconnect(telux::wlan::Id::PRIMARY);

    std::cout << "\nDisconnecting from External AP"
            << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
            << ". ErrorCode: " << static_cast<int>(retCode)
            << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanStaInterfaceManagerMenu::manageStaService(std::vector<std::string> userInput) {

    std::cout << "\nManage Station Service" << std::endl;

    int opr = 0;
    std::cout << "Select STA Service Operation\
            (0-STOP, 1-START, 2-RESTART): ";
    std::cin >> opr;
    WlanUtils::validateInput(opr, {0, 1, 2});
    std::cout << std::endl;

    telux::common::ErrorCode retCode = wlanStaInterfaceManager_->manageStaService(
        telux::wlan::Id::PRIMARY, static_cast<telux::wlan::ServiceOperation>(opr));

    std::cout << "Manage Station Service Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanStaInterfaceManagerMenu::onStationStatusChanged(
    std::vector<telux::wlan::StaStatus> status) {
    PRINT_NOTIFICATION << " ** Wlan onStationStatusChange **\n";
    WlanUtils::printStaStatus(status);
}

void WlanStaInterfaceManagerMenu::onScanResultUpdated(
    const telux::wlan::StaScanResult &staScanResult) {
    PRINT_NOTIFICATION << " ** Wlan onScanResultUpdated **\n";
    WlanUtils::printScanResult(staScanResult);
}

void WlanStaInterfaceManagerMenu::onStationBandChanged(telux::wlan::BandType radio) {
   PRINT_NOTIFICATION << " ** Wlan onStationOperationBandChanged **\n";

   if(radio == telux::wlan::BandType::BAND_2GHZ) {
       std::cout << "Station has switched to 2G band" << std::endl;
   } else if(radio == telux::wlan::BandType::BAND_5GHZ) {
       std::cout << "Station has switched to 5G band" << std::endl;
   } else {
       std::cout << "Station has switched to 6G band" << std::endl;
   }
}


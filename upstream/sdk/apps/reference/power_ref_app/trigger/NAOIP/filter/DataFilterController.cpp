/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
extern "C"
{
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/data/DataFactory.hpp>
#include <telux/common/DeviceConfig.hpp>

#include "../../../common/RefAppUtils.hpp"

#include "DataFilterController.hpp"
#define PROTO_TCP 6
#define PROTO_UDP 17

using namespace telux::data::net;

DataFilterController::DataFilterController() {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slots_ = MAX_SLOT_ID;
    } else {
        slots_ = DEFAULT_SLOT_ID;
    }
}

DataFilterController::~DataFilterController() {
    LOG(DEBUG, __FUNCTION__);

    for(SlotId i = SLOT_ID_1; i <= slots_;  i = static_cast<SlotId>(static_cast<int>(i) + 1)) {
        if (dataFilterListener_ && dataFilterMgrMap_[i]) {
            dataFilterMgrMap_[i]->deregisterListener(dataFilterListener_);
        }
        dataFilterMgrMap_[i] = nullptr;
    }
    dataFilterListener_ = nullptr;
}

bool DataFilterController::initializeSDK() {
    LOG(INFO, __FUNCTION__, " isDataFilterMgrReady = ", (int)isDataFilterMgrReady_);
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    std::weak_ptr<DataFilterController> weakFromThis = shared_from_this();

    // Get the DataFactory instances.
    auto &dataFactory = telux::data::DataFactory::getInstance();

    do {
        if (!isDataFilterMgrReady_) {
            for(SlotId i = SLOT_ID_1; i <= slots_;
                i = static_cast<SlotId>(static_cast<int>(i) + 1)) {
                // data filter mananger
                std::promise<telux::common::ServiceStatus> dfsProm;
                // Get data filter manager object
                dataFilterMgrMap_[i] =
                    dataFactory.getDataFilterManager(i,
                                                    [&dfsProm](telux::common::ServiceStatus status)
                                                    { dfsProm.set_value(status); });
                if (!dataFilterMgrMap_[i]) {
                    LOG(ERROR, __FUNCTION__, " Failed to get DataFilterManager object");
                    break;
                }

                //wait for filter manager to get ready
                LOG(DEBUG, __FUNCTION__, " Initializing Data filter manager subsystem Please wait");
                subSystemStatus = dfsProm.get_future().get();
                if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                    LOG(DEBUG, __FUNCTION__, " Data Filter Manager is ready");
                    dataFilterListener_ = std::make_shared<DataFilterListener>(weakFromThis);
                    if (!dataFilterListener_) {
                        LOG(ERROR, __FUNCTION__, " unable to instantiate data filter listener");
                    }
                    telux::common::Status status =
                        dataFilterMgrMap_[i]->registerListener(dataFilterListener_);
                    if (status != telux::common::Status::SUCCESS) {
                        LOG(ERROR, __FUNCTION__,
                            " Unable to register data filter manager listener");
                    }
                } else {
                    LOG(ERROR, __FUNCTION__, " Data Filter Manager is failed");
                    dataFilterMgrMap_[i] = nullptr;
                    return false;
                }
            }
            isDataFilterMgrReady_ = true;
            // To be sure deactivating filer while starting
            telux::data::DataRestrictMode mode;
            mode.filterMode = DataRestrictModeType::DISABLE;
            sendSetDataRestrictMode(mode);
        }
    } while (0);

    LOG(INFO, __FUNCTION__, " isDataFilterMgrReady = ", (int)isDataFilterMgrReady_);
    return isDataFilterMgrReady_;
}

void DataFilterController::registerListener(std::weak_ptr<IDataFilterListener> listner) {
    LOG(DEBUG, __FUNCTION__);
    for(SlotId i = SLOT_ID_1; (i <= slots_) && dataFilterMgrMap_[i];
        i = static_cast<SlotId>(static_cast<int>(i) + 1)) {
        telux::common::Status status = dataFilterMgrMap_[i]->registerListener(listner);
        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Unable to register data filter manager listener");
            break;
        }
    }
}

bool DataFilterController::sendSetDataRestrictMode(DataRestrictMode mode) {
    LOG(DEBUG, __FUNCTION__);
    if (!isDataFilterMgrReady_) {
        LOG(ERROR, __FUNCTION__, " Data restrict filter feature is not supported.");
        return false;
    }

    if (mode.filterMode == DataRestrictModeType::ENABLE) {
        LOG(DEBUG, __FUNCTION__, "  Sending command to enable Data Filter");
    } else if (mode.filterMode == DataRestrictModeType::DISABLE) {
        LOG(DEBUG, __FUNCTION__, "  Sending command to disable Data Filter");
    }

    if (mode.filterAutoExit == DataRestrictModeType::ENABLE) {
        LOG(DEBUG, __FUNCTION__, " auto exit is enable");
    }

    for(SlotId i = SLOT_ID_1; i <= slots_; i = static_cast<SlotId>(static_cast<int>(i) + 1)) {
        // data filter mananger
        std::promise<telux::common::ErrorCode> prom;
        telux::common::Status status = telux::common::Status::FAILED;
        // Get data filter manager object
        status = dataFilterMgrMap_[i]->setDataRestrictMode(mode, [&prom](ErrorCode errorCode) {
            if (errorCode == telux::common::ErrorCode::SUCCESS) {
                LOG(DEBUG," sendSetDataRestrictMode command success callback ");
            } else {
                LOG(ERROR," sendSetDataRestrictMode command failed callback");
            };
            prom.set_value(errorCode);
        });

        if (status != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, "  *** ERROR - Failed to send Data Restrict command");
            return false;
        } else {
            telux::common::ErrorCode errCode = prom.get_future().get();
            if (errCode != telux::common::ErrorCode::SUCCESS) {
                LOG(ERROR, __FUNCTION__, " callback Error = ",
                    RefAppUtils::getErrorCodeAsString(errCode));
                    return false;
            }
        }
    }
    return true;
}

IpProtocol DataFilterController::getTypeOfFilter(
    DataConfigParser instance, std::map<std::string, std::string> filter) {
    LOG(DEBUG, __FUNCTION__);
    IpProtocol type = 0;
    if (instance.getValue(filter, "FILTER_PROTOCOL_TYPE") != "") {
        std::string protoType = instance.getValue(filter, "FILTER_PROTOCOL_TYPE");
        if (strcmp(protoType.c_str(), "UDP") == 0) {
            type = PROTO_UDP;
        } else if (strcmp(protoType.c_str(), "TCP") == 0) {
            type = PROTO_TCP;
        }
        LOG(DEBUG, __FUNCTION__, " protocol : ", protoType);
    }
    return type;
}

SlotId DataFilterController::getSlotIdOfFilter(
    DataConfigParser instance, std::map<std::string, std::string> filter) {
    LOG(DEBUG, __FUNCTION__);
    SlotId slotId = DEFAULT_SLOT_ID;
    if (instance.getValue(filter, "SLOT_ID") != "") {
        std::string slotStr = instance.getValue(filter, "SLOT_ID");
        if (strcmp(slotStr.c_str(), "2") == 0) {
            slotId = SLOT_ID_2;
        }
        LOG(DEBUG, __FUNCTION__, " slot id : ", slotStr);
    }
    return slotId;
}

void DataFilterController::addIPParameters(
    std::shared_ptr<telux::data::IIpFilter> &dataFilter, DataConfigParser instance,
    std::map<std::string, std::string> filterMap) {
    LOG(DEBUG, __FUNCTION__);
    if (instance.getValue(filterMap, "SOURCE_IPV4_ADDRESS") != "" ||
        instance.getValue(filterMap, "DESTINATION_IPV4_ADDRESS") != "") {
        telux::data::IPv4Info ipv4Info_ = {};
        if (instance.getValue(filterMap, "SOURCE_IPV4_ADDRESS") != "") {
            ipv4Info_.srcAddr = instance.getValue(filterMap, "SOURCE_IPV4_ADDRESS");
        }
        if (instance.getValue(filterMap, "DESTINATION_IPV4_ADDRESS") != "") {
            ipv4Info_.destAddr = instance.getValue(filterMap, "DESTINATION_IPV4_ADDRESS");
        }
        dataFilter->setIPv4Info(ipv4Info_);
    }

    if (instance.getValue(filterMap, "SOURCE_IPV6_ADDRESS") != "" ||
        instance.getValue(filterMap, "DESTINATION_IPV6_ADDRESS") != "") {
        telux::data::IPv6Info ipv6Info_ = {};
        if (instance.getValue(filterMap, "SOURCE_IPV6_ADDRESS") != "") {
            ipv6Info_.srcAddr = instance.getValue(filterMap, "SOURCE_IPV6_ADDRESS");
        }
        if (instance.getValue(filterMap, "DESTINATION_IPV6_ADDRESS") != "") {
            ipv6Info_.destAddr = instance.getValue(filterMap, "DESTINATION_IPV6_ADDRESS");
        }
        dataFilter->setIPv6Info(ipv6Info_);
    }
}

int DataFilterController::getPortInfo(DataConfigParser cfgParser,
                                std::map<std::string, std::string> pairMap, std::string key,
                                std::string errorStr) {
    LOG(DEBUG, __FUNCTION__);
    int value = std::stoi(cfgParser.getValue(pairMap, key));

    if (value > std::numeric_limits<unsigned short>::max() ||
        value < std::numeric_limits<unsigned short>::min()) {
        throw invalid_argument(errorStr);
    }
    return value;
}

bool DataFilterController::addFilter(std::vector<std::shared_ptr<Connection>> connectionList) {
    LOG(DEBUG, __FUNCTION__);
    if(RefAppUtils::isDataFilterInstallationEnabled()) {
        telux::common::Status retStat = telux::common::Status::SUCCESS;
        if (!isDataFilterMgrReady_) {
            LOG(ERROR, __FUNCTION__, " data filter manager is not ready ");
            return false;
        }
        // Add data filter for socket connections
        for(auto connection: connectionList) {
            LOG(DEBUG, __FUNCTION__, " connection: ", connection->toString());
            std::shared_ptr<telux::data::IIpFilter> dataFilter =
                configureConnectionToDataFilter(connection);

            std::promise<telux::common::ErrorCode> prom{};
            retStat = dataFilterMgrMap_[connection->slotId]
                ->addDataRestrictFilter(dataFilter, [&prom] (telux::common::ErrorCode error) {
                    prom.set_value(error);
                }
            );
            if (retStat == telux::common::Status::SUCCESS) {
                telux::common::ErrorCode errCode = prom.get_future().get();
                if (errCode != telux::common::ErrorCode::SUCCESS) {
                    LOG(ERROR, __FUNCTION__, " callback Error = ",
                        RefAppUtils::getErrorCodeAsString(errCode));
                }
            } else {
                LOG(ERROR, __FUNCTION__, " Error = ", RefAppUtils::teluxStatusToString(retStat));
            }
        }
    }
    return addFilter();
}

// Add data filter from config file
bool DataFilterController::addFilter() {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status retStat = telux::common::Status::SUCCESS;

    bool isSuccess = true;
    do {
        if (!isDataFilterMgrReady_) {
            LOG(ERROR, __FUNCTION__, " data filter manager is not ready ");
            break;
        }

        string configFilterFile =
            ConfigParser::getInstance()->getValue("NAOIP_TRIGGER", "NAOIP_FILTER_CONFIG_FILE");
        if (configFilterFile.empty()) {
            configFilterFile = DEFAULT_DATA_CONFIG_FILE_NAME;
        }

        DataConfigParser cfgParser("filter", configFilterFile);
        std::vector<std::map<std::string, std::string>> vectorFilter = cfgParser.getFilters();

        LOG(DEBUG, __FUNCTION__, " Total Filter = ", vectorFilter.size());
        for (uint8_t i = 0; i < vectorFilter.size(); i++) {

            IpProtocol typeOfFilter = getTypeOfFilter(cfgParser, vectorFilter[i]);
            std::shared_ptr<telux::data::IIpFilter> dataFilter;

            if (typeOfFilter == PROTO_TCP) {
                dataFilter = configureTCPFilter(cfgParser, vectorFilter[i]);
            } else if (typeOfFilter == PROTO_UDP) {
                dataFilter = configureUDPFilter(cfgParser, vectorFilter[i]);
            } else {
                LOG(DEBUG, __FUNCTION__, "  *** ERROR - Invalid conf file parameters");
                isSuccess = false;
                continue;
            }
            LOG(DEBUG, __FUNCTION__, "  Sending command to Add Data Filter");
            std::promise<telux::common::ErrorCode> prom{};
            retStat = dataFilterMgrMap_[getSlotIdOfFilter(cfgParser, vectorFilter[i])]
                ->addDataRestrictFilter(dataFilter, [&prom] (telux::common::ErrorCode error) {
                    prom.set_value(error);
                }
            );
            if (retStat == telux::common::Status::SUCCESS) {
                telux::common::ErrorCode errCode = prom.get_future().get();
                if (errCode != telux::common::ErrorCode::SUCCESS) {
                    LOG(ERROR, __FUNCTION__, " callback Error = ",
                        RefAppUtils::getErrorCodeAsString(errCode));
                        isSuccess = false;
                        continue;
                }
            } else {
                LOG(ERROR, __FUNCTION__, " Error = ", RefAppUtils::teluxStatusToString(retStat));
                isSuccess = false;
                continue;
            }
        }
    } while(0);
    LOG(INFO, __FUNCTION__, " add data filter status " );
    return isSuccess;

}

std::shared_ptr<telux::data::IIpFilter> DataFilterController::configureConnectionToDataFilter(
    std::shared_ptr<Connection> connection) {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<telux::data::IIpFilter> dataFilter;

    if (connection->protocol == Protocol::TCP) {
        dataFilter = telux::data::DataFactory::getInstance().getNewIpFilter(PROTO_TCP);
        auto tcpRestrictFilter = std::dynamic_pointer_cast<ITcpFilter>(dataFilter);

        telux::data::TcpInfo tcpInfo = {};
        if(connection->connectionRole == ConnectionRole::CLIENT) {
            tcpInfo.src.port = connection->serverPort;
        } else {
            tcpInfo.src.port = connection->clientPort;
        }

        tcpInfo.src.range = 0;
        tcpInfo.dest.range = 0;

        if (tcpRestrictFilter) {
            tcpRestrictFilter->setTcpInfo(tcpInfo);
        } else {
            LOG(ERROR, __FUNCTION__, "  *** ERROR - Invalid tcp filter");
        }
    } else if (connection->protocol == Protocol::UDP) {
        dataFilter = telux::data::DataFactory::getInstance().getNewIpFilter(PROTO_UDP);
        auto udpRestrictFilter = std::dynamic_pointer_cast<IUdpFilter>(dataFilter);

        telux::data::UdpInfo udpInfo = {};
        if(connection->connectionRole == ConnectionRole::CLIENT) {
            udpInfo.src.port = connection->serverPort;
        } else {
            udpInfo.src.port = connection->clientPort;
        }

        udpInfo.src.range = 0;
        udpInfo.dest.range = 0;

        if (udpRestrictFilter) {
            udpRestrictFilter->setUdpInfo(udpInfo);
        } else {
            LOG(ERROR, __FUNCTION__, "  *** ERROR - Invalid udp filter");
        }
    }

    if (connection->ipFamily == telux::data::IpFamilyType::IPV4) {
        telux::data::IPv4Info ipv4Info = {};
        if(connection->connectionRole == ConnectionRole::CLIENT) {
            ipv4Info.srcAddr = connection->serverIpAddr;
            ipv4Info.destAddr = connection->clientIpAddr;
        } else {
            ipv4Info.srcAddr = connection->clientIpAddr;
            ipv4Info.destAddr = connection->serverIpAddr;
        }

        dataFilter->setIPv4Info(ipv4Info);
    } else if (connection->ipFamily == telux::data::IpFamilyType::IPV6) {
        telux::data::IPv6Info ipv6Info = {};
        if(connection->connectionRole == ConnectionRole::CLIENT) {
            ipv6Info.srcAddr = connection->serverIpAddr;
            ipv6Info.destAddr = connection->clientIpAddr;
        } else {
            ipv6Info.srcAddr = connection->clientIpAddr;
            ipv6Info.destAddr = connection->serverIpAddr;
        }
        dataFilter->setIPv6Info(ipv6Info);
    } else {
        LOG(ERROR, __FUNCTION__, "  *** ERROR - Invalid IP family");
    }
    return dataFilter;
}

std::shared_ptr<telux::data::IIpFilter> DataFilterController::configureTCPFilter(
    DataConfigParser cfgParser, std::map<std::string, std::string> filter) {
    LOG(DEBUG, __FUNCTION__, " Creating TCP filter ");
    // Get data filter manager object
    std::shared_ptr<telux::data::IIpFilter> dataFilter =
        telux::data::DataFactory::getInstance().getNewIpFilter(PROTO_TCP);
    addIPParameters(dataFilter, cfgParser, filter);
    auto tcpRestrictFilter = std::dynamic_pointer_cast<ITcpFilter>(dataFilter);

    telux::data::TcpInfo tcpInfo_ = {};
    tcpInfo_.src.range = 0;
    tcpInfo_.dest.range = 0;
    try {
        if (cfgParser.getValue(filter, "TCP_SOURCE_PORT") != "") {
            tcpInfo_.src.port = getPortInfo(cfgParser, filter, "TCP_SOURCE_PORT",
                                            "TCP port value");
            if (cfgParser.getValue(filter, "TCP_SOURCE_PORT_RANGE") != "") {
                tcpInfo_.src.range =
                    getPortInfo(cfgParser, filter,"TCP_SOURCE_PORT_RANGE", "TCP Port range value");
            }
        }

        if (cfgParser.getValue(filter, "TCP_DESTINATION_PORT") != "") {
            tcpInfo_.dest.port =
                getPortInfo(cfgParser, filter,
                            "TCP_DESTINATION_PORT", "TCP port value");
            if (cfgParser.getValue(filter, "TCP_DESTINATION_PORT_RANGE") != "") {
                tcpInfo_.dest.range =
                    getPortInfo(cfgParser, filter,
                                "TCP_DESTINATION_PORT_RANGE", "TCP port range value");
            }
        }
    } catch (const std::exception &e) {
        LOG(ERROR, __FUNCTION__, "  *** ERROR - Invalid ", string(e.what()),
            ", expected in range (0-65535)");
    }

    if (tcpRestrictFilter) {
        tcpRestrictFilter->setTcpInfo(tcpInfo_);
    } else {
        LOG(ERROR, __FUNCTION__, "  *** ERROR - Invalid tcp filter");
    }

    return dataFilter;
}

std::shared_ptr<telux::data::IIpFilter> DataFilterController::configureUDPFilter(
    DataConfigParser cfgParser, std::map<std::string, std::string> filter ) {
    LOG(DEBUG, __FUNCTION__, " Creating UDP filter ");

    // Get data filter manager object
    std::shared_ptr<telux::data::IIpFilter> dataFilter =
        telux::data::DataFactory::getInstance().getNewIpFilter(PROTO_UDP);
    addIPParameters(dataFilter, cfgParser, filter);

    auto udpRestrictFilter = std::dynamic_pointer_cast<IUdpFilter>(dataFilter);
    telux::data::UdpInfo udpInfo_ = {};
    udpInfo_.src.range = 0;
    udpInfo_.dest.range = 0;
    try {
        if (cfgParser.getValue(filter, "UDP_SOURCE_PORT") != "") {
            udpInfo_.src.port =
                getPortInfo(cfgParser, filter, "UDP_SOURCE_PORT",
                            "UDP port value");
            if (cfgParser.getValue(filter, "UDP_SOURCE_PORT_RANGE") != "") {
                udpInfo_.src.range =
                    getPortInfo(cfgParser, filter,
                                "UDP_SOURCE_PORT_RANGE", "UDP Port range value");
            }
        }

        if (cfgParser.getValue(filter, "UDP_DESTINATION_PORT") != "") {
            udpInfo_.dest.port = getPortInfo(cfgParser, filter,
                                                "UDP_DESTINATION_PORT", "UDP port value");
            if (cfgParser.getValue(filter, "UDP_DESTINATION_PORT_RANGE") != "") {
                udpInfo_.dest.range =
                    getPortInfo(cfgParser, filter,
                                "UDP_DESTINATION_PORT_RANGE", "UDP port range vlaue");
            }
        }
    }
    catch (const std::exception &e) {
        LOG(ERROR, __FUNCTION__, "  *** ERROR - Invalid ", string(e.what()),
        ", expected in range (0-65535)");
    }
    if (udpRestrictFilter) {
        udpRestrictFilter->setUdpInfo(udpInfo_);
    } else {
        LOG(ERROR, __FUNCTION__, "  *** ERROR - Invalid udp filter");
    }

    return dataFilter;
}

bool DataFilterController::removeAllFilter() {
    LOG(DEBUG, __FUNCTION__);
    if (!isDataFilterMgrReady_) {
        LOG(DEBUG, __FUNCTION__, " Data restrict filter feature is not supported.");
        return false;
    }
    LOG(DEBUG, __FUNCTION__, " Remove data filters");
    bool isSuccess = true;
    for(SlotId i = SLOT_ID_1; i <= slots_;  i = static_cast<SlotId>(static_cast<int>(i) + 1)) {
        telux::common::Status status = telux::common::Status::FAILED;
        promise<ErrorCode> prom;
        status = dataFilterMgrMap_[i]->removeAllDataRestrictFilters([&](ErrorCode errorCode) {
            if (errorCode == telux::common::ErrorCode::SUCCESS) {
                LOG(DEBUG," removeAllFilter command success callback");
            } else {
                LOG(ERROR," removeAllFilter command failed callback");
            };
            prom.set_value(errorCode);
        });

        if (status == telux::common::Status::SUCCESS) {
            telux::common::ErrorCode errCode = prom.get_future().get();
            if (errCode != telux::common::ErrorCode::SUCCESS) {
                LOG(ERROR, __FUNCTION__, " callback Error = ",
                    RefAppUtils::getErrorCodeAsString(errCode));
                    isSuccess = false;
            }
        } else {
            LOG(ERROR, __FUNCTION__, " Error = ", RefAppUtils::teluxStatusToString(status));
            isSuccess = false;

        }
    }
    return isSuccess;
}

DataFilterController::DataFilterListener::DataFilterListener(std::weak_ptr<DataFilterController>
                        dataController) : dataController_(dataController) {
    LOG(DEBUG, __FUNCTION__);
}

void DataFilterController::DataFilterListener::onDataRestrictModeChange(DataRestrictMode mode) {
    LOG(DEBUG, __FUNCTION__);
    if (mode.filterMode == DataRestrictModeType::ENABLE) {
        LOG(DEBUG, __FUNCTION__,"Data Filter Mode : Enable");
    } else if (mode.filterMode == DataRestrictModeType::DISABLE) {
        LOG(DEBUG, __FUNCTION__, "Data Filter Mode : Disable");
    } else {
        LOG(ERROR, __FUNCTION__, " ERROR: Invalid Data Filter mode notified");
    }
}

void DataFilterController::DataFilterListener::onServiceStatusChange(
    telux::common::ServiceStatus status) {

    LOG(DEBUG, __FUNCTION__, " DataFilterListener status = ",
        RefAppUtils::serviceStatusToString(status));

    bool dfmStatus = status == telux::common::ServiceStatus::SERVICE_AVAILABLE ? true : false;
    if (std::shared_ptr<DataFilterController> dataController = dataController_.lock()) {
        dataController->isDataFilterMgrReady_ = dfmStatus;
    } else {
        LOG(ERROR, __FUNCTION__, " unable to lock dataController");
    }

    LOG(INFO, __FUNCTION__, " isDataFilterMgrReady_ = ",
        (int)dfmStatus);
}

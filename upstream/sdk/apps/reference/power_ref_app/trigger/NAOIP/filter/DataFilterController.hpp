/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef DATAFILTERCONTROLLER_HPP
#define DATAFILTERCONTROLLER_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <iomanip>

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/DataFilterManager.hpp>
#include <telux/data/DataFilterListener.hpp>
#include <telux/common/Log.hpp>

#include "DataConfigParser.hpp"


using namespace telux::data;
using namespace telux::common;
using namespace telux::data::net;
using namespace std;

/**
 * @brief Data filter controller deals with communication with telsdk with respect to filter
 * management and connection information
 *
 */

class DataFilterController : public enable_shared_from_this<DataFilterController>
{
public:
    bool initializeSDK();

    // Data Filter APIs
    bool sendSetDataRestrictMode(DataRestrictMode mode);
    bool addFilter();
    bool addFilter(std::vector<std::shared_ptr<Connection>> connectionList);
    std::shared_ptr<telux::data::IIpFilter> configureConnectionToDataFilter(
        std::shared_ptr<Connection> connection);
    bool removeAllFilter();

    IpProtocol getTypeOfFilter(DataConfigParser instance,
        std::map<std::string, std::string> filter);
    SlotId getSlotIdOfFilter(
        DataConfigParser instance, std::map<std::string, std::string> filter);
    void addIPParameters(std::shared_ptr<telux::data::IIpFilter> &dataFilter,
        DataConfigParser instance, std::map<std::string, std::string> filterMap);
    ResponseCallback responseCb;
    void commandCallback(ErrorCode errorCode);
    int getPortInfo(DataConfigParser cfgParser, std::map<std::string, std::string> pairMap,
        std::string key, std::string errorStr);
    std::shared_ptr<telux::data::IIpFilter> configureTCPFilter( DataConfigParser cfgParser,
        std::map<std::string, std::string> filter);
    std::shared_ptr<telux::data::IIpFilter> configureUDPFilter(DataConfigParser cfgParser,
        std::map<std::string, std::string> filter);
    bool isUDP();
    void registerListener(std::weak_ptr<IDataFilterListener> listner);

    DataFilterController();
    ~DataFilterController();

private:
    int slots_ = 0;
    bool isDataFilterMgrReady_ = false;
    std::condition_variable cvDataFilterMgrReady_;
    std::map<SlotId, std::shared_ptr<telux::data::IDataFilterManager>> dataFilterMgrMap_;

    /** Listener to update change in data filter info */
    class DataFilterListener : public telux::data::IDataFilterListener {
        std::weak_ptr<DataFilterController> dataController_;
        public:
        DataFilterListener(std::weak_ptr<DataFilterController> dataController);

        void onDataRestrictModeChange(DataRestrictMode mode) override;
        void onServiceStatusChange(telux::common::ServiceStatus status) override;
    };
    std::shared_ptr<DataFilterListener> dataFilterListener_;
};
#endif

/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef NAOIPTRIGGER_HPP
#define NAOIPTRIGGER_HPP

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <map>
#include <memory>
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <string>
#include <chrono>

#include <telux/power/TcuActivityDefines.hpp>
#include <telux/common/Log.hpp>

#include "DataFilterController.hpp"
#include "RefAppUtils.hpp"
#include "common/define.hpp"
#include "ConfigParser.hpp"
#include "Event.hpp"
#include "EventManager.hpp"
#include "TCPKeepAliveHandler.hpp"
#include "IEventListener.hpp"
#include "ConnectionHandler.hpp"

/**
 * @brief NAOIpTrigger class watches for TCU state change events in IP packets. controls data
 * filtering according to triggered status.
 */

class NAOIpTrigger :    public IEventListener ,
                        public ISocketConnectionListener,
                        public IDataFilterListener,
                        public IDataConnectionListener,
                        public enable_shared_from_this<NAOIpTrigger> {
private:
    bool isUDP_ = false;
    std::map<string, TcuActivityState> triggerText_;        /** map which stores trigger text with
                                                                respect to TcuActivityState */

    ConfigParser * config_;     /** config parser to fetch data from config file */
    std::shared_ptr<EventManager> eventManager_ = nullptr;            /** event management */
    std::shared_ptr<DataFilterController> dataFilterController_ = nullptr;
                        /** controller for data call and data filters */

    std::shared_ptr<ConnectionHandler> connectionHandler_ = nullptr;
    std::shared_ptr<TCPKeepAliveHandler>  tcpKeepAliveHandler_ = nullptr;

    std::condition_variable messageCv_;
    std::mutex messageMtx_;

    bool validateTrigger(char* buffer, int length,
      TcuActivityState& tcuActivityState, std::string& machineName);
    bool loadConfig();
    void triggerEvent(TcuActivityState event, std::string machineName);
    bool enableFilter();
    bool disableFilter();

public:
    NAOIpTrigger(std::shared_ptr<EventManager> eventManager);
    bool init();
    void onEventRejected(shared_ptr<Event> event, EventStatus reason) override;
    void onEventProcessed(shared_ptr<Event> event, bool success) override;

    void messageReceived(IPMessage msg, int length,
      std::shared_ptr<Connection> connection) override;

    void onDataCallInfoChanged(
      const std::shared_ptr<telux::data::IDataCall> &dataCall) override;

    void onDataRestrictModeChange(DataRestrictMode mode) override;

    ~NAOIpTrigger();
};


#endif
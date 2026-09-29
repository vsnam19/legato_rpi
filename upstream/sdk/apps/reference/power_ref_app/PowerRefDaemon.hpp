/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef POWERREFDAEMON_HPP
#define POWERREFDAEMON_HPP

#include <grp.h>
#include <sys/types.h>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <ostream>
#include <telux/common/CommonDefines.hpp>
#include <telux/power/TcuActivityDefines.hpp>
#include <telux/power/PowerFactory.hpp>
#include <telux/power/TcuActivityManager.hpp>
#include <telux/power/TcuActivityListener.hpp>
#include <telux/common/Log.hpp>
#include "common/define.hpp"
#include "Event.hpp"
#include "EventManager.hpp"
#include "trigger/NAOIP/NAOIpTrigger.hpp"
#include "trigger/SMS/SMSTrigger.hpp"
#include "../../common/utils/Utils.hpp"

#ifdef CAN_TRIGGER_SUPPORTED
#include "trigger/CAN/CANTrigger.hpp"
#endif // CAN_TRIGGER_SUPPORTED

#ifdef TELSDK_FEATURE_SATCOM_ENABLED
#include "NtnClient.hpp"
#endif

using namespace telux::power;
using namespace telux::common;
/**
 * @brief PowerRefDaemon class initialise all triggers, EventManager instance and handles signal
 *
 */

class PowerRefDaemon {
public:
   telux::common::Status init();
   int startDaemon(int argc, char **argv);
   void stopDaemon();
   void registerForUpdates();

   static PowerRefDaemon &getInstance();

private:
   std::mutex mtx_;
   std::condition_variable cv_;
   std::atomic<bool> exiting_ = {false};
   ConfigParser* config_;
   shared_ptr<EventManager> eventManager_;
   shared_ptr<NAOIpTrigger> naoIpTrigger_;
   shared_ptr<SMSTrigger> smsTrigger_;

   #ifdef CAN_TRIGGER_SUPPORTED
   shared_ptr<CANTrigger> canTrigger_;
   #endif // CAN_TRIGGER_SUPPORTED

#ifdef TELSDK_FEATURE_SATCOM_ENABLED
   bool ntnEnabled_ = false;
   std::shared_ptr<NtnClient> ntnClient_ = nullptr;
#endif

   static void signalHandler(int signum);
   void printUsage(char **argv);
   telux::common::Status parseArguments(int argc, char **argv);
};

#endif
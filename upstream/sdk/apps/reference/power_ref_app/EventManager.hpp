/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef EVENTMANAGER_HPP
#define EVENTMANAGER_HPP

#include <memory>
#include <iostream>
#include <deque>
#include <vector>
#include <map>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <telux/power/TcuActivityDefines.hpp>
#include <telux/power/PowerFactory.hpp>
#include <telux/power/TcuActivityManager.hpp>
#include <telux/power/TcuActivityListener.hpp>
#include <telux/common/Log.hpp>

#include "common/define.hpp"
#include "Event.hpp"
#include "IEventListener.hpp"

using namespace telux::power;
using namespace telux::common;
using namespace std;

/**
 * @brief The EventManager class controls the execution sequence of events. The Event Manager
 * executes events with the help of TcuActivityManager (telsdk) and the controlling node dealing
 * with power state.
 *
 */

class EventManager : public ITcuActivityListener,
                     public IServiceStatusListener,
                     public enable_shared_from_this<EventManager>
{

   static EventManager *instance;

private:
   mutex cleanup_;
   deque<shared_ptr<Event>> eventQueue_;
   mutex eventQueueUpdate_;

   shared_ptr<ITcuActivityManager> tcuActivityStateMgr_;
   map<TriggerType, vector<weak_ptr<IEventListener>>> eventListeners_;
   TcuActivityState localState_ = TcuActivityState::RESUME;
   std::string localMachineName_;

   EventManager();
   void eventSchedule(shared_ptr<Event> event);
   bool registerTcuActivityManager();
   void printTcuActivityState(TcuActivityState state);
   void printQueue();

   void setActivityState(shared_ptr<Event> event);
   void notifyOnEventRejected(shared_ptr<Event> event, EventStatus status);
   void notifyAndEraseEventProcessed(TriggerType triggerType, TcuActivityState triggeredState,
                                     bool success, EventStatus status);

   // wake lock node control
   void writeToSystemNode(char *nodepath, char *value, int length);

public:
   ~EventManager();
   static EventManager *getInstance();
   bool init();

   // interact with TcuActivityManager
   void onTcuActivityStateUpdate(TcuActivityState state, std::string machineName) override;
   void onSlaveAckStatusUpdate(const telux::common::Status status,
      const std::string machineName, const std::vector<ClientInfo> unresponsiveClients,
      const std::vector<ClientInfo> nackResponseClients) override;
   void onServiceStatusChange(telux::common::ServiceStatus status) override;

   // event management
   void pushEvent(shared_ptr<Event> event);
   // remove and notify 0th event and another event in queue triggered for the same TCU state
   void processedEventHandler(EventStatus status);

   // Event listener
   void registerListener(weak_ptr<IEventListener> eventListener,
                         TriggerType triggerType = TriggerType::UNKNOWN);
   void updateEventStatus(shared_ptr<Event> event, bool processed, bool succeed,
                          EventStatus status);
   TcuActivityState getActivityState();
   void holdWakeLock();
   void holdWakeLock(const std::string& wakeLockValue);
   void releaseWakeLock();
   void releaseWakeLock(const std::string& wakeLockValue);
   void cleanup();
};

#endif
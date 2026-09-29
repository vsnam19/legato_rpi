/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef POWERREF_DEFINES_HPP
#define POWERREF_DEFINES_HPP

#include <telux/data/DataDefines.hpp>

#include "ISocketConnectionListener.hpp"

#include <thread>
#include <iostream>

#define DAEMON_NAME           "telux_power_refd"
#define WAKE_LOCK             "temp_telux_power_refd"

/** concerned node with regard to power state */
#define WAKELOCK_PATH         "/sys/power/wake_lock"
#define WAKEUNLOCK_PATH       "/sys/power/wake_unlock"
#define AUTOSLEEP_NODE_PATH   "/sys/power/autosleep"

#define AUTOSLEEP_NODE_OFF    "off"
#define AUTOSLEEP_NODE_MEM    "mem"

#define TRIGGER_SUSPEND       "TRIGGER_SUSPEND"
#define TRIGGER_RESUME        "TRIGGER_RESUME"
#define TRIGGER_SHUTDOWN      "TRIGGER_SHUTDOWN"

#define MACHINE_NAME_DELIMINATOR            ':'

/**
 * Represents the inputs/triggers that the daemon listens to, for initiating TcuActivity state
 * transition
 */
enum TriggerType {
  NAOIP_TRIGGER = 1,
  SMS_TRIGGER,
  GPIO_TRIGGER,
  CAN_TRIGGER,
  UNKNOWN
};

enum EventStatus {
  INITIALIZED = 1,                    /** trigger is initialized */
  IN_QUEUE,                           /** trigger is added to the queue */
  IN_PROGRESS_TCU_ACTIVITY,           /** trigger passed to TCU activity manager */
  REJECTED_INVALID_STATE_TRANSITION,  /** trigger rejected due to invalid state transition */
  REJECTED_INVALID_MACHINE_NAME,      /** trigger rejected due to invalid machine name */
  REJECTED_EVENT_OVERRIDDEN,          /** trigger is overridden */
  FAILED_TCU_ACTIVITY,                /** TCU activity state transition failed */
  FAILED_TCU_ACTIVITY_TIMEOUT,        /** TCU activity state transition failed due to timeout */
  SUCCEED                             /** TCU activity state transition succeeded */
};

#endif //POWERREF_DEFINES_HPP

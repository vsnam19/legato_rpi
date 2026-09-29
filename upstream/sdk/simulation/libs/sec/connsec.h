/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CONNSEC_H_
#define CONNSEC_H_

#include <string>
#include <vector>
#include <cstdint>

typedef enum {
    CATEGORY_FLAG_UNKNOWN = 0,
    CATEGORY_FLAG_IMSI_LEAK = 1,
    CATEGORY_FLAG_IMPRISONER = 2,
    CATEGORY_FLAG_DOS = 4,
    CATEGORY_FLAG_DOWNGRADE = 8,
    CATEGORY_FLAG_LOCATION_TRACKER= 16,
    CATEGORY_FLAG_ATTRACTIVE = 32,
    CATEGORY_FLAG_AUTH_PASSED = 64,
    CATEGORY_FLAG_NO_ENCRYPTION = 128,
    CATEGORY_FLAG_WEAK_ENCRYPTION = 256,
    CATEGORY_FLAG_SELF_BLACKLISTING_CELL = 512,
    CATEGORY_FLAG_UNAUTH_SMS = 1024,
    CATEGORY_FLAG_UNAUTH_EMERGENCY_MSG = 2048,
    CATEGORY_FLAG_CS_CALL_CONNECTED = 4096,
    CATEGORY_FLAG_REJECT_SENT = 8192,
    CATEGORY_FLAG_LOCATION_TRACKER_AUTH_REQEUST = 16384
} ssgccs_client_category_flag_t;

typedef enum environmental_state {
    CCS_UNKNOWN,
    SAFE,
    ALERT,
    HOSTILE
} ssgccs_environmental_state_t;

typedef enum {
    POLICY_NO_ACTION,
    POLICY_DEPRIORITIZE,
    POLICY_BAR,
    POLICY_UNDEPRIORITIZE,
    POLICY_UNBAR,
    POLICY_ABNORMAL
} ssgccs_client_policy_action_t;

typedef enum radio_enum {
    RADIO_UKNOWN,
    RADIO_1,
    RADIO_GERAN,
    RADIO_WCDMA,
    RADIO_LTE,
    RADIO_NR
} ssgccs_radio_enum_t;

typedef enum {
    SSGCCS_OFFLINE,
    SSGCCS_ONLINE
} ssgccs_state_t;

typedef struct {
    uint32_t indication_count = 0;
    uint32_t hostile_score_count = 0;
    std::string session_start_time = "";
    float average_score = 0;
    uint32_t categories_detected = 0;
    std::string most_recent_policy_acted = "";
    int8_t was_countermeasure_enacted = 0;
} ssgccs_session_state_t;

#endif /* CONNSEC_H_ */

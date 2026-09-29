/*
 *  Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       Helper.hpp
 *
 * @brief
 *
 */

#ifndef HELPER_HPP
#define HELPER_HPP

#include <telux/tel/SmsManager.hpp>
#include <telux/tel/PhoneDefines.hpp>

enum TelEventType {
    UNKNOWN,
    SMS_MEMORY_FULL,
    SMS_INCOMING
};

enum CallApi {
    makeECallWithMsd = 0,
    makeTpsECallOverCSWithMsd = 1,
    makeTpsECallOverIMS = 2,
    makeECallWithRawMsd = 3,
    makeTpsECallOverCSWithRawMsd = 4,
    makeECallWithoutMsd = 5,
    makeTpsECallOverCSWithoutMsd = 6,
    updateEcallMsd = 7,
    updateECallRawMsd = 8,
    makeVoiceCall = 9,
    makeRttVoiceCall = 10,
    makeSelfTestERAGLONASSECallWithRawMsd = 11
};

class Helper  {
public:
    static telux::tel::SmsTagType getTagType(std::string tagType ) {
        if(tagType == "MT_READ") {
            return telux::tel::SmsTagType::MT_READ;
        } else if (tagType == "MT_NOT_READ") {
            return telux::tel::SmsTagType::MT_NOT_READ;
        } else {
            return telux::tel::SmsTagType::UNKNOWN;
        }
    }

    static telux::tel::SmsEncoding getencodingMethod(std::string encoding ) {
        if(encoding == "GSM7") {
            return telux::tel::SmsEncoding::GSM7;
        } else if (encoding == "GSM8") {
            return telux::tel::SmsEncoding::GSM8;
        } else if (encoding == "UCS2") {
            return telux::tel::SmsEncoding::UCS2;
        } else {
            return telux::tel::SmsEncoding::UNKNOWN;
        }
    }

    static std::string tagTypeToString(telux::tel::SmsTagType type) {
        switch(type) {
            case telux::tel::SmsTagType::MT_READ:
                return "MT_READ";
            case telux::tel::SmsTagType::MT_NOT_READ:
                return "MT_NOT_READ";
            case telux::tel::SmsTagType::UNKNOWN:
                return "UNKNOWN";
        }
        return "UNKNOWN";
    }

    static telux::tel::StorageType getstorageType(std::string storageType ) {
        if(storageType == "SIM") {
            return telux::tel::StorageType::SIM;
        } else if (storageType == "NONE") {
            return telux::tel::StorageType::NONE;
        } else {
            return telux::tel::StorageType::UNKNOWN;
        }
    }

    static std::string storageTypeToString(telux::tel::StorageType type) {
        switch(type) {
            case telux::tel::StorageType::SIM:
                return "SIM";
            case telux::tel::StorageType::NONE:
                return "NONE";
            case telux::tel::StorageType::UNKNOWN:
                return "UNKNOWN";
        }
        return "UNKNOWN";
    }

    static std::string encodingToString(telux::tel::SmsEncoding encoding) {
        switch(encoding) {
            case telux::tel::SmsEncoding::GSM7:
                return "GSM7";
            case telux::tel::SmsEncoding::GSM8:
                return "GSM8";
            case telux::tel::SmsEncoding::UCS2:
                return "UCS2";
            case telux::tel::SmsEncoding::UNKNOWN:
                return "UNKNOWN";
        }
        return "UNKNOWN";
    }

    static telux::tel::CallState getCallState(std::string callState ) {
        if(callState == "CALL_IDLE") {
            return telux::tel::CallState::CALL_IDLE;
        } else if (callState == "CALL_ACTIVE") {
            return telux::tel::CallState::CALL_ACTIVE;
        } else if (callState == "CALL_HOLD") {
            return telux::tel::CallState::CALL_ON_HOLD;
        } else if (callState == "CALL_DIALING") {
            return telux::tel::CallState::CALL_DIALING;
        } else if (callState == "CALL_INCOMING") {
            return telux::tel::CallState::CALL_INCOMING;
        } else if (callState == "CALL_WAITING") {
            return telux::tel::CallState::CALL_WAITING;
        } else if (callState == "CALL_ALERTING") {
            return telux::tel::CallState::CALL_ALERTING;
        } else if (callState == "CALL_ENDED") {
            return telux::tel::CallState::CALL_ENDED;
        } else {
            return telux::tel::CallState::CALL_IDLE;
        }
    }

    static std::string getCallStateInString(telux::tel::CallState callState) {
        if(callState == telux::tel::CallState::CALL_IDLE) {
            return "CALL_IDLE";
        } else if (callState == telux::tel::CallState::CALL_ACTIVE) {
            return "CALL_ACTIVE";
        } else if (callState == telux::tel::CallState::CALL_ON_HOLD) {
            return "CALL_HOLD";
        } else if (callState == telux::tel::CallState::CALL_DIALING) {
            return "CALL_DIALING";
        } else if (callState == telux::tel::CallState::CALL_INCOMING) {
            return "CALL_INCOMING";
        } else if (callState == telux::tel::CallState::CALL_WAITING) {
            return "CALL_WAITING";
        } else if (callState == telux::tel::CallState::CALL_ALERTING) {
            return "CALL_ALERTING";
        } else if (callState == telux::tel::CallState::CALL_ENDED) {
            return "CALL_ENDED";
        } else {
            return "CALL_IDLE";
        }
    }
};

#endif // HELPER_HPP
/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * Utility helper class
 * @brief RefAppUtils class performs common error code conversions
 */

#ifndef POWER_REF_APP_UTILS_HPP
#define POWER_REF_APP_UTILS_HPP

#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

#include <telux/data/DataFactory.hpp>
#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/ServingSystemManager.hpp>
#include <telux/common/Log.hpp>
#include <telux/power/TcuActivityDefines.hpp>
#include "ISocketConnectionListener.hpp"
#include "define.hpp"

class RefAppUtils {
 public:

    /**
     * Get error description for given ErrorCode
     */
    static std::string getErrorCodeAsString(telux::common::ErrorCode error);
    static std::map<telux::common::ErrorCode, std::string> errorCodeToStringMap_;
    static TriggerType stringToTriggerType(std::string stringTrigger);
    static bool stringToProtocol(std::string protocol);

    static telux::data::IpProtocol getProtcol(std::string protoStr);
    static int callEndReasonCode(telux::data::DataCallEndReason ceReason);
    static std::string callEndReasonTypeToString(telux::data::EndReasonType type);
    static std::string techPreferenceToString(telux::data::TechPreference techPref);
    static std::string ipFamilyTypeToString(telux::data::IpFamilyType ipType);
    static std::string dataCallStatusToString(telux::data::DataCallStatus dcStatus);
    static std::string bearerTechToString(telux::data::DataBearerTechnology bearerTech);
    static std::string operationTypeToString(telux::data::OperationType oprType);
    static std::string protocolToString(telux::data::IpProtocol proto);
    static std::string drbStatusToString(telux::data::DrbStatus stat);
    static std::string serviceRatToString(telux::data::NetworkRat rat);
    static std::string eventStatusToString(EventStatus status);
    static std::string tcuActivityStateToString(telux::power::TcuActivityState state);
    static std::string teluxStatusToString(telux::common::Status status);
    static std::string triggerTypeToString(TriggerType triggeredBy);
    static std::string serviceStatusToString(telux::common::ServiceStatus status);
    static std::string dataRestrictModeTypeToString(telux::data::DataRestrictModeType filterMode);
    static std::vector<std::shared_ptr<Connection>> getConnectionConfigs();
    static uint32_t getKeepAliveInterval();

    static bool isUDP();
    static bool isClient();
    static bool isKeepAliveEnabled();
    static bool isAutoExitEnabled();
    static bool isDataFilterInstallationEnabled();
 private:
    static bool stringToBool(std::string enable);
};

#endif

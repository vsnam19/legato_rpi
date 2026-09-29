/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * WlanUtility helper class
 * @brief WlanUtils class performs common functions in Wlan.
 */

#ifndef WLANUTILS_HPP
#define WLANUTILS_HPP

#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>
#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanFactory.hpp>
#include "../../common/utils/Utils.hpp"

class WlanUtils {
public:

   template <typename T>
   static void validateInput(T& input, std::initializer_list<T> list) {
      return Utils::validateInput(input, list);
   }
   static std::string getWlanDeviceName(telux::wlan::HwDeviceType device);
   static std::string getWlanApType(telux::wlan::ApType apType);
   static std::string getWlanId(telux::wlan::Id id);
   static std::string getStaInterfaceStatus(telux::wlan::StaInterfaceStatus status);
   static std::string getStaConnectionStatus(telux::wlan::StaConnectionStatus status);
   static std::string apAccessToString(telux::wlan::ApInterworking interworking);
   static std::string RadioTypeToString(telux::wlan::BandType radio);
   static std::string apSecurityModeToString(telux::wlan::SecMode mode);
   static std::string apSecurityAuthToString(telux::wlan::SecAuth auth);
   static std::string apSecurityEncryptToString(telux::wlan::SecEncrypt encrypt);
   static void printAPStatus(std::vector<telux::wlan::ApStatus>& apStatus);
   static void printStaStatus(std::vector<telux::wlan::StaStatus>& staStatus);
   static void printDeviceInfo(std::vector<telux::wlan::DeviceInfo>& info);
   static void printApElementInfo(telux::wlan::ApElementInfoConfig ElementInfoConfig);
   static void printNetworkConfigs(
      std::vector<telux::wlan::StaNetworkConfigInfo> networkConfigsInfo);
   static void printScanResult(const telux::wlan::StaScanResult &staScanResult);
   static std::string apElementInfoAccessTypeToString(telux::wlan::NetAccessType accessType);
   static telux::wlan::Id convertIntToWlanId(int id);
   static telux::wlan::ApType convertIntToApType(int type);
   static telux::wlan::ApInterworking convertIntToInterworking(int interworking);
   static telux::wlan::SecMode convertIntToSecMode(int mode);
   static telux::wlan::SecAuth convertIntToSecAuth(int auth);
   static telux::wlan::SecEncrypt convertIntToSecEncrypt(int encrypt);
};

#endif

/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef NTNTESTAPP_HPP
#define NTNTESTAPP_HPP

#include <memory>
#include <atomic>
#include <map>

#include "ConsoleApp.hpp"
#include <telux/satcom/NtnManager.hpp>
#ifdef TELSDK_FEATURE_LOC_ENABLED
#include <telux/loc/LocationManager.hpp>
#include <telux/loc/LocationListener.hpp>
#include <telux/loc/LocationDefines.hpp>
#endif

using namespace telux::satcom;
using namespace telux::common;
#ifdef TELSDK_FEATURE_LOC_ENABLED
using namespace telux::loc;
class NtnLocationListener;
#endif

class NtnTestApp : public INtnListener,
                   public ConsoleApp,
                   public std::enable_shared_from_this<NtnTestApp> {
 public:
    NtnTestApp();
    ~NtnTestApp();

    void registerForUpdates();
    void deregisterForUpdates();
    void consoleInit();

    std::string toString(NtnState state);
    std::string toString(NtnCapabilities cap);
    std::string toString(SignalStrength ss);
    std::string toString(ServiceStatus status);
    std::string toString(LocationFixRequestReason reqReason);
    void onIncomingData(std::unique_ptr<uint8_t[]> data, uint32_t size);
    void onNtnStateChange(NtnState state);
    void onCapabilitiesChange(NtnCapabilities capabilities);
    void onSignalStrengthChange(SignalStrength newStrength);
    void onServiceStatusChange(ServiceStatus status);
    void onDataAck(ErrorCode err, TransactionId id);
    void onCellularCoverageAvailable(bool isCellularCoverageAvailable);
    void onLocationFixRequest(LocationFixRequestReason reqReason);
    void onNtnBandUpdate(uint32_t bandValue);
    void getServiceStatus(std::vector<std::string> inputCommand);
    void isNtnSupported(std::vector<std::string> inputCommand);
    void enableNtn(std::vector<std::string> inputCommand);
    void sendDataString(std::vector<std::string> inputCommand);
    void sendDataRaw(std::vector<std::string> inputCommand);
    void abortData(std::vector<std::string> inputCommand);
    void getNtnCapabilities(std::vector<std::string> inputCommand);
    void getSignalStrength(std::vector<std::string> inputCommand);
    void updateSystemSelectionSpecifiers(std::vector<std::string> inputCommand);
    void getNtnState(std::vector<std::string> inputCommand);
    void enableCellularScan(std::vector<std::string> inputCommand);
    void setLocationFix(std::vector<std::string> inputCommand);
    void locationFixResponse(std::vector<std::string> inputCommand);
    void autoSetLocationFixFromFile(std::vector<std::string> inputCommand);
    void stopAutoSetLocationFixFromFile(std::vector<std::string> inputCommand);
    void setLiveLocationFix();
#ifdef TELSDK_FEATURE_LOC_ENABLED
    void triggerLocationReports();
    bool initLocationManager();
#endif
    // Member variable to keep the manager object alive till application ends.
    std::shared_ptr<telux::satcom::INtnManager> ntnMgr_ = nullptr;
#ifdef TELSDK_FEATURE_LOC_ENABLED
    std::shared_ptr<ILocationManager> locationManager_ = nullptr;
    std::shared_ptr<NtnLocationListener> posListener_ = nullptr;
#endif

private:
    std::atomic<bool> autoSetLocationFix_ {false};
#ifdef TELSDK_FEATURE_LOC_ENABLED
    std::atomic<bool> setLiveLocationFix_ {false};
#endif
    std::atomic<int> lastIndex_ {0};
    std::atomic<int> currentIndex_ {0};
    std::vector<std::map<std::string, std::string>> rows_;

    void autoSetLocationFix();
    void updateLocationFixFromRow(telux::satcom::LocationFix& fixParams);
    void readData(const std::string& filename,
        std::vector<std::map<std::string, std::string>>& rows);

};

#ifdef TELSDK_FEATURE_LOC_ENABLED
class NtnLocationListener : public telux::loc::ILocationListener {
  public:
    void onDetailedLocationUpdate(
        const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) override;
    std::mutex locMtx_;
    std::condition_variable locCv_;
    bool isReportReceived_ = false;
    telux::satcom::LocationFix locFix_;
    //Needed to prevent overwriting of the first snapshot data received via the updates.
    int reportCount_ = 0;
};
#endif

#endif  // NTNTESTAPP_HPP

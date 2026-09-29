/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef NTNCLIENT_HPP
#define NTNCLIENT_HPP

#include <telux/common/Log.hpp>
#include <telux/satcom/NtnManager.hpp>

#ifdef TELSDK_FEATURE_LOC_ENABLED
#include <telux/loc/LocationManager.hpp>
#include <telux/loc/LocationListener.hpp>
#include <telux/loc/LocationDefines.hpp>
#endif

#include "common/ConfigParser.hpp"
#include "../../common/utils/Utils.hpp"

using namespace telux::satcom;
using namespace telux::common;
#ifdef TELSDK_FEATURE_LOC_ENABLED
using namespace telux::loc;
class NtnClientLocationListener;
#endif

class NtnClient : public INtnListener,
                    public std::enable_shared_from_this<NtnClient> {
 public:
    NtnClient();
    ~NtnClient();

    telux::common::Status init();

    void registerForUpdates();
    void deregisterForUpdates();

    telux::common::ErrorCode enableNtn();

    void onNtnStateChange(NtnState state);
    std::string toString(NtnState state);

    void onSignalStrengthChange(SignalStrength newStrength);
    std::string toString(SignalStrength ss);

    void onCapabilitiesChange(NtnCapabilities capabilities);
    std::string toString(NtnCapabilities cap);

    void onNtnBandUpdate(uint32_t bandValue);

    void onLocationFixRequest(LocationFixRequestReason reqReason);
    std::string toString(LocationFixRequestReason reqReason);

#ifdef TELSDK_FEATURE_LOC_ENABLED
    void triggerLocationReports();
    bool initLocationManager();
#endif

    telux::common::Status sendDataString(std::string text);
    void onDataAck(ErrorCode err, TransactionId id);

    void cleanup();

 private:
    bool initSatcom();

    // Member variable to keep the manager object alive till application ends.
    std::shared_ptr<telux::satcom::INtnManager> ntnMgr_ = nullptr;
#ifdef TELSDK_FEATURE_LOC_ENABLED
    std::shared_ptr<ILocationManager> locationManager_ = nullptr;
    std::shared_ptr<NtnClientLocationListener> posListener_ = nullptr;
#endif
};

#ifdef TELSDK_FEATURE_LOC_ENABLED
class NtnClientLocationListener : public telux::loc::ILocationListener {
  public:
    void onDetailedLocationUpdate(
        const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) override;
    // Provide controlled access methods
    std::mutex& getLocationMutex() { return locMtx_; }
    std::condition_variable& getLocationCV() { return locCv_; }
    telux::satcom::LocationFix locFix_;
    bool isReportReceived_ = false;
    //Needed to prevent overwriting of the first snapshot data received via the updates.
    int reportCount_ = 0;

  private:
    std::mutex locMtx_;
    std::condition_variable locCv_;
};
#endif

#endif  // NTNCLIENT_HPP
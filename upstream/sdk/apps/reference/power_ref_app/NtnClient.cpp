/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <csignal>
#include <mutex>
#include <condition_variable>
#include <string>
#include <iomanip>
#include <ios>
#include <sstream>
#include <thread>

#include "NtnClient.hpp"
#include <telux/satcom/SatcomFactory.hpp>
#ifdef TELSDK_FEATURE_LOC_ENABLED
#include <telux/loc/LocationFactory.hpp>
#endif

#define DEFAULT_REPORT_MASK 0
#define DEFAULT_REPORT_INTERVAL 1000

/**
 * @file: NtnClient.cpp
 *
 * @brief: Perform ntn operations.
 */

NtnClient::NtnClient()
   : ntnMgr_(nullptr) {
}

NtnClient::~NtnClient() {
}

telux::common::Status NtnClient::init() {
    LOG(DEBUG, __FUNCTION__);
    if(ntnMgr_ == nullptr) {
        if(!initSatcom()) {
            LOG(ERROR, __FUNCTION__, " Ntn manager init failed");
            return telux::common::Status::FAILED;
        } else {
            LOG(DEBUG, __FUNCTION__, " Ntn manager init success");
        }
    }

#ifdef TELSDK_FEATURE_LOC_ENABLED
    if(locationManager_ == nullptr) {
        if(!initLocationManager()) {
            LOG(ERROR, __FUNCTION__, " Location manager init failed");
            return telux::common::Status::FAILED;
        } else {
            LOG(DEBUG, __FUNCTION__, " Location manager init success");
        }
    }
#endif
    return telux::common::Status::SUCCESS;
}

bool NtnClient::initSatcom() {
    if(ntnMgr_ == nullptr) {
        auto &satcomFactory = telux::satcom::SatcomFactory::getInstance();
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        ntnMgr_ = satcomFactory.getNtnManager([&](telux::common::ServiceStatus status)
                                                    { prom.set_value(status); });
        if (ntnMgr_ == nullptr) {
            LOG(ERROR, __FUNCTION__, " satcomFactory.getNtnManager returned nullptr");
            return false;
        }
        ServiceStatus ntnMgrStatus = ntnMgr_->getServiceStatus();
        if(ntnMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
            LOG(DEBUG, __FUNCTION__, " Ntn subsystem is not ready, Please wait");
        }
        ntnMgrStatus = prom.get_future().get();
        if (ntnMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(DEBUG, __FUNCTION__, " *** Ntn subsystem is ready ***");
        } else {
            LOG(ERROR, __FUNCTION__, " *** Ntn subsystem is not ready ***");
            return false;
        }
    }
    return true;
}

#ifdef TELSDK_FEATURE_LOC_ENABLED
bool NtnClient::initLocationManager() {
    bool locSubsystemReady = true;
    if(locationManager_ == nullptr) {
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        auto &locationFactory = LocationFactory::getInstance();
        locationManager_ = locationFactory.getLocationManager([&](ServiceStatus status) {
            if (status == ServiceStatus::SERVICE_AVAILABLE) {
                    prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
                } else {
                    prom.set_value(ServiceStatus::SERVICE_FAILED);
                }
            });
        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
        startTime = std::chrono::system_clock::now();
        ServiceStatus locMgrStatus = locationManager_->getServiceStatus();
        if(locMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
            LOG(DEBUG, __FUNCTION__, " Location subsystem is not ready, Please wait");
        }
        locMgrStatus = prom.get_future().get();
        if(locMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LOG(DEBUG, __FUNCTION__, " Elapsed Time for Subsystems to ready : ",
                elapsedTime.count());
        } else {
            LOG(ERROR, __FUNCTION__, " ERROR - Unable to initialize Location subsystem");
            locSubsystemReady = false;
        }

        posListener_ = std::make_shared<NtnClientLocationListener>();
        locationManager_->registerListenerEx(posListener_);
    }
    return locSubsystemReady;
}
#endif

void NtnClient::registerForUpdates() {
    LOG(DEBUG, __FUNCTION__);
    Status status = ntnMgr_->registerListener(shared_from_this());
    if (status != Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " ERROR - Failed to register for ntn notification");
    } else {
        LOG(DEBUG, __FUNCTION__, " Registered Listener for ntn notification");
    }
}

void NtnClient::deregisterForUpdates() {
    LOG(DEBUG, __FUNCTION__);
    Status status = ntnMgr_->deregisterListener(shared_from_this());
    if (status != Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " ERROR - Failed to deregister for ntn notification");
    } else {
        LOG(DEBUG, __FUNCTION__, " Deregistered Listener");
    }
}

telux::common::ErrorCode NtnClient::enableNtn() {
    LOG(DEBUG, __FUNCTION__);
    int emergency = 0;
    std::string iccid = "";
    ConfigParser* config = ConfigParser::getInstance();
    if (config->getValue("NTN_CONFIGS", "EMERGENCY") == "TRUE") {
        emergency = 1;
    }
    iccid = config->getValue("NTN_CONFIGS","ICCID");
    return ntnMgr_->enableNtn(true, emergency, iccid);
}

void NtnClient::onNtnStateChange(NtnState state) {
    LOG(DEBUG, __FUNCTION__, "**** onNtnStateChange = ", toString(state));
}

std::string NtnClient::toString(NtnState state) {
    switch (state) {
        case NtnState::DISABLED:
            return "DISABLED";
        case NtnState::OUT_OF_SERVICE:
            return "OUT_OF_SERVICE";
        case NtnState::IN_SERVICE:
            return "IN_SREVICE";
    }
    return "-";
}

void NtnClient::onSignalStrengthChange(SignalStrength newStrength) {
    LOG(DEBUG, __FUNCTION__, "**** onSignalStrengthChange = ", toString(newStrength));
}

std::string NtnClient::toString(SignalStrength ss) {
    switch (ss) {
        case SignalStrength::NONE:
            return "NONE";
        case SignalStrength::POOR:
            return "POOR";
        case SignalStrength::MODERATE:
            return "MODERATE";
        case SignalStrength::GOOD:
            return "GOOD";
        case SignalStrength::GREAT:
            return "GREAT";
    }
    return "-";
}

void NtnClient::onCapabilitiesChange(NtnCapabilities capabilities) {
    LOG(DEBUG, __FUNCTION__, "**** onCapabilitiesChange = ", toString(capabilities));
}

std::string NtnClient::toString(NtnCapabilities cap) {
    return std::to_string(cap.maxDataSize);
}

void NtnClient::onNtnBandUpdate(uint32_t bandValue) {
    LOG(DEBUG, __FUNCTION__, "**** onNtnBandUpdate BandValue = ", bandValue);
}

void NtnClient::onLocationFixRequest(LocationFixRequestReason reqReason) {
    LOG(DEBUG, __FUNCTION__, "**** onLocationFixRequest Reason = ", toString(reqReason));

    std::thread ([this] {
        #ifdef TELSDK_FEATURE_LOC_ENABLED
            triggerLocationReports();
        #endif
    }).detach();
}

std::string NtnClient::toString(LocationFixRequestReason reqReason) {
    switch (reqReason) {
        case LocationFixRequestReason::NORMAL:
            return "NORMAL";
        case LocationFixRequestReason::VALIDITY_TIMER_EXPIRED:
            return "VALIDITY_TIMER_EXPIRED";
        case LocationFixRequestReason::UNKOWN:
            return "UNKOWN";
    }
    return "-";
}

#ifdef TELSDK_FEATURE_LOC_ENABLED
void NtnClient::triggerLocationReports() {
    LOG(DEBUG, __FUNCTION__);
    if(locationManager_ && posListener_) {
        GnssReportTypeMask reportMask = DEFAULT_REPORT_MASK;
        reportMask |= LOCATION;
        auto startStatus = locationManager_->startDetailedReports(
            DEFAULT_REPORT_INTERVAL, nullptr, reportMask);
        if(startStatus != telux::common::Status::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Failed to start location reports");
            return;
        }
        {
            LOG(DEBUG, __FUNCTION__, " Waiting for location reports");
            std::unique_lock<std::mutex> lck(posListener_->getLocationMutex());
            posListener_->getLocationCV().wait(lck, [this]{ return posListener_->isReportReceived_; });
        }
        locationManager_->stopReports(nullptr);
        {
            std::unique_lock<std::mutex> lck(posListener_->getLocationMutex());
            posListener_->isReportReceived_ = false;
            posListener_->reportCount_ = 0;
        }
        LOG(DEBUG, __FUNCTION__, " Stopping reports");
        auto err = ntnMgr_->locationFixResponse(telux::satcom::LocationStatus::SUCCESS, 0);
        LOG(DEBUG, __FUNCTION__, " locationFixResponse err = ", Utils::getErrorCodeAsString(err));

        err = ntnMgr_->setLocationFix(posListener_->locFix_);
        LOG(DEBUG, __FUNCTION__, " setLocationFix err = ", Utils::getErrorCodeAsString(err));
    }
}

void NtnClientLocationListener::onDetailedLocationUpdate(
    const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) {
    if(reportCount_ > 0) {
        return;
    }
    std::cout << " Detailed reports received" << std::endl;
    LOG(DEBUG, __FUNCTION__);
    locFix_.lat = locationInfo->getLatitude();
    locFix_.lon = locationInfo->getLongitude();
    locFix_.alt = locationInfo->getAltitude();
    locFix_.uncerCircular = locationInfo->getHorizontalUncertainty();
    locFix_.heading = locationInfo->getHeading();
    locFix_.headingUncer = locationInfo->getHeadingUncertainty();
    std::vector<float> velocityEastNorthUp;
    locationInfo->getVelocityEastNorthUp(velocityEastNorthUp);
    size_t itr = 0;
    for(auto vel: velocityEastNorthUp) {
        locFix_.velInfo.enuVel[itr] = vel;
        itr++;
    }
    std::vector<float> velocityUncertaintyEastNorthUp;
    locationInfo->getVelocityUncertaintyEastNorthUp(velocityUncertaintyEastNorthUp);
    itr = 0;
    for(auto vel: velocityUncertaintyEastNorthUp) {
        locFix_.velInfo.enuUncer[itr] = vel;
        itr++;
    }
    telux::loc::LocationInfoValidity validityMask = locationInfo->getLocationInfoValidity();
    telux::loc::LocationInfoExValidity validityMaskEx = locationInfo->getLocationInfoExValidity();
    if((validityMask & telux::loc::HAS_HEADING_BIT)) {
        locFix_.isHeadingValid = true;
    } else {
        locFix_.isHeadingValid = false;
    }
    if((validityMask & telux::loc::HAS_HEADING_ACCURACY_BIT)) {
        locFix_.isHeadingUncerValid = true;
    } else {
        locFix_.isHeadingUncerValid = false;
    }
    if((validityMask & telux::loc::HAS_HORIZONTAL_ACCURACY_BIT)) {
        locFix_.isConfidenceValid = true;
        locFix_.confidence = 68;
    } else {
        locFix_.isConfidenceValid = false;
        locFix_.confidence = 0;
    }
    if((validityMaskEx & telux::loc::HAS_NORTH_VEL) && (validityMaskEx & telux::loc::HAS_EAST_VEL)
        && (validityMaskEx & telux::loc::HAS_UP_VEL)) {
        locFix_.velInfo.isEnuValueValid = true;
    } else {
        locFix_.velInfo.isEnuValueValid = false;
    }
    if((validityMaskEx & telux::loc::HAS_NORTH_VEL_UNC) &&
        (validityMaskEx & telux::loc::HAS_EAST_VEL_UNC)
            && (validityMaskEx & telux::loc::HAS_UP_VEL_UNC)) {
        locFix_.velInfo.isEnuUncerValid = true;
    } else {
        locFix_.velInfo.isEnuUncerValid = false;
    }
    {
        std::unique_lock<std::mutex> lck(this->getLocationMutex());
        isReportReceived_ = true;
        reportCount_++;
        this->getLocationCV().notify_all();
    }
}
#endif

telux::common::Status NtnClient::sendDataString(std::string text) {
    LOG(DEBUG, __FUNCTION__);
    int emergency = 0;
    ConfigParser* config = ConfigParser::getInstance();
    if (config->getValue("NTN_CONFIGS", "IS_EMERGENCY_DATA") == "TRUE") {
        emergency = 1;
    }
    std::vector<uint8_t> data;
    for (char c : text) {
        data.push_back((uint8_t) c);
    }
    TransactionId tId;
    LOG(DEBUG, __FUNCTION__, " Data of size : ", data.size());
    auto ret = ntnMgr_->sendData(data.data(), text.size(), emergency, tId);
    LOG(DEBUG, __FUNCTION__, " sendData tId = ", tId);
    return ret;
}

void NtnClient::onDataAck(ErrorCode err, TransactionId id) {
    LOG(DEBUG, __FUNCTION__);
    if (err == ErrorCode::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " **** onDataAck ack received for id = ", id);
    } else {
        LOG(DEBUG, __FUNCTION__, " **** onDataAck error = ",
            Utils::getErrorCodeAsString(err), " id = ", id);
    }
}

void NtnClient::cleanup() {
    deregisterForUpdates();
    LOG(DEBUG, __FUNCTION__);
    if(ntnMgr_) {
        ntnMgr_ = nullptr;
    }
#ifdef TELSDK_FEATURE_LOC_ENABLED
    if(locationManager_ && posListener_) {
        locationManager_->deRegisterListenerEx(posListener_);
        posListener_ = nullptr;
        locationManager_ = nullptr;
    }
#endif
}
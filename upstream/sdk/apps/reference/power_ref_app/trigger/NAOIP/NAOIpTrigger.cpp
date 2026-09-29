/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "NAOIpTrigger.hpp"
#include "common/RefAppUtils.hpp"

NAOIpTrigger::NAOIpTrigger(std::shared_ptr<EventManager> eventManager) {
    LOG(DEBUG, __FUNCTION__);
    eventManager_ = eventManager;
}

NAOIpTrigger::~NAOIpTrigger() {
    LOG(DEBUG, __FUNCTION__);
    eventManager_ = nullptr;
    dataFilterController_ = nullptr;
    if (connectionHandler_) {
        connectionHandler_->cleanup();
        connectionHandler_ = nullptr;
    }
    tcpKeepAliveHandler_ = nullptr;
}

bool NAOIpTrigger::init() {
    LOG(DEBUG, __FUNCTION__);
    config_ = ConfigParser::getInstance();
    bool returnValue = false;
    do {
        if (!loadConfig()) {
            break;
        }
        weak_ptr<NAOIpTrigger> weakFromThis = shared_from_this();
        if(!eventManager_) {
            LOG(ERROR, __FUNCTION__, "  event manager is not available ");
            break;
        }
        // Connection handler initialisation
        connectionHandler_ = ConnectionHandler::getInstance();
        if(!connectionHandler_){
            return false;
        }
        std::shared_ptr<ISocketConnectionListener> listener = shared_from_this();
        // connectionHandler_->registerListener(listener);
        std::vector<std::shared_ptr<Connection>> connectionConfigList =
            RefAppUtils::getConnectionConfigs();
        connectionHandler_->start(connectionConfigList);
        for (const auto &connection : connectionConfigList) {
            if(connection->socketConnection && connection->dataConnectionManager) {
                connection->socketConnection->registerListener(shared_from_this());
                connection->dataConnectionManager->registerListener(shared_from_this());
            } else {
                LOG(ERROR, __FUNCTION__, " error on connection");
                return false;
            }
        }
        if (!RefAppUtils::isUDP() && RefAppUtils::isKeepAliveEnabled()) {
            tcpKeepAliveHandler_ = TCPKeepAliveHandler::getInstance(eventManager_);
            if (tcpKeepAliveHandler_ && tcpKeepAliveHandler_->init()) {
                LOG(DEBUG, __FUNCTION__, " naoIpTrigger init succeed");
            } else {
                LOG(ERROR, __FUNCTION__, " naoIpTrigger init failed");
                return false;
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " keep alive is not enabled");
        }

        dataFilterController_ = std::make_shared<DataFilterController>();
        if (dataFilterController_ ) {
            returnValue = dataFilterController_->initializeSDK();
            dataFilterController_->registerListener(weakFromThis);
            if (returnValue) {
                //Listen to all triggers to be able to add and remove data filters.
                eventManager_->registerListener(weakFromThis, TriggerType::UNKNOWN);
                break;
            } else {
                //telsdk initialisation failed wait for some time and retry
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            }
        } else {
            LOG(ERROR, __FUNCTION__, "  Unable to instantiate data controller ");
        }

    } while (0);

    return returnValue;
}

bool NAOIpTrigger::enableFilter() {
    LOG(DEBUG, __FUNCTION__);
    if (dataFilterController_) {
        if (!dataFilterController_->addFilter(connectionHandler_->getConnectionList())) {
            LOG(ERROR, __FUNCTION__, " addFilter failed");
        }
        DataRestrictMode mode;
        // Note: If filter auto exit is enabled, it will disable the filter if any packets pass
        //       through a whitelisted filter, even if it is an unexpected packet.
        //       @ref DataRestrictMode
        // ex. mode.filterAutoExit = DataRestrictModeType::ENABLE;
        if(RefAppUtils::isAutoExitEnabled()) {
            mode.filterAutoExit = DataRestrictModeType::ENABLE;
        } else {
            mode.filterAutoExit = DataRestrictModeType::DISABLE;
        }
        mode.filterMode = DataRestrictModeType::ENABLE;

        if (dataFilterController_->sendSetDataRestrictMode(mode)) {
            return true;
        }
        LOG(ERROR, __FUNCTION__, " sendSetDataRestrictMode failed");
    } else {
        LOG(ERROR, __FUNCTION__, " dataFilterController is not ready");
    }
    return false;
}

bool NAOIpTrigger::disableFilter() {
    LOG(DEBUG, __FUNCTION__);
    if (dataFilterController_) {
        DataRestrictMode mode;
        mode.filterMode = DataRestrictModeType::DISABLE;
        if (!dataFilterController_->sendSetDataRestrictMode(mode)) {
            LOG(ERROR, __FUNCTION__, " sendSetDataRestrictMode is failed");
        }
        return true;
    } else {
        LOG(ERROR, __FUNCTION__, " dataFilterController is not ready");
    }
    return false;
}

void NAOIpTrigger::onEventRejected(shared_ptr<Event> event, EventStatus reason) {
    LOG(DEBUG, __FUNCTION__, " reason = ", (int)reason);
    if (event->getTriggeredState() == TcuActivityState::SUSPEND
        && reason == EventStatus::REJECTED_INVALID_STATE_TRANSITION) {
        if (tcpKeepAliveHandler_) {
            tcpKeepAliveHandler_->startKAOffload();
        }
        enableFilter();
    }
    if (event->getTriggeredState() == TcuActivityState::RESUME
        && reason == EventStatus::REJECTED_INVALID_STATE_TRANSITION) {
        if (tcpKeepAliveHandler_) {
            tcpKeepAliveHandler_->stopKAOffload();
        }
        disableFilter();
    }
}

void NAOIpTrigger::onEventProcessed(shared_ptr<Event> event, bool success) {
    LOG(DEBUG, __FUNCTION__);

    if (success) {
        if (event->getTriggeredState() == TcuActivityState::SUSPEND) {
            if(tcpKeepAliveHandler_) {
                tcpKeepAliveHandler_->startKAOffload();
            }
            enableFilter();
        } else if (event->getTriggeredState() == TcuActivityState::RESUME) {
            if(tcpKeepAliveHandler_) {
                tcpKeepAliveHandler_->stopKAOffload();
            }
            disableFilter();
        }
    }
}

void NAOIpTrigger::triggerEvent(TcuActivityState eventState, std::string machineName) {
    LOG(DEBUG, __FUNCTION__);

    std::shared_ptr<Event> event = std::make_shared<Event>(eventState, machineName,
        TriggerType::NAOIP_TRIGGER);
    if ( event ) {
        if(eventManager_) {
            eventManager_->pushEvent(event);
        } else {
            LOG(ERROR, __FUNCTION__, "  event manager is not available ");
        }
    } else {
        LOG(ERROR, __FUNCTION__, " unable to create event");
    }

}

bool NAOIpTrigger::validateTrigger(char *buffer, int length,
    TcuActivityState& tcuActivityState, std::string& machineName) {
    LOG(DEBUG, __FUNCTION__);
    string text(buffer, length);
    // to avoid \n in a string which might lead to not matching trigger text
    text.erase(std::remove(text.begin(), text.end(), '\n'), text.cend());
    LOG(DEBUG, __FUNCTION__, text);
    size_t deliminatorPosition = 0;
    if(( deliminatorPosition = text.find(MACHINE_NAME_DELIMINATOR)) != std::string::npos ) {
        machineName = text.substr(deliminatorPosition + sizeof(MACHINE_NAME_DELIMINATOR),
            text.length());
        text = text.substr(0, deliminatorPosition);
    }
    if (triggerText_.find(text) == triggerText_.end()) {
        LOG(ERROR, __FUNCTION__, " invalid trigger text, text = ", text);
    } else {
        LOG(INFO, __FUNCTION__, " valid trigger text, text = ", text);
        tcuActivityState = triggerText_[text];
        return true;
    }
    return false;
}

void NAOIpTrigger::onDataRestrictModeChange(DataRestrictMode mode){
    LOG(DEBUG, __FUNCTION__);
    if(mode.filterMode == DataRestrictModeType::DISABLE) {
        eventManager_->holdWakeLock("DataFilterDisabled");
        std::unique_lock<std::mutex> lock(messageMtx_);
        if (messageCv_.wait_for(lock, std::chrono::seconds(2)) == std::cv_status::timeout) {
            if(eventManager_->getActivityState() == TcuActivityState::SUSPEND) {
                if(tcpKeepAliveHandler_) {
                    tcpKeepAliveHandler_->stopKAOffload();
                    tcpKeepAliveHandler_->startKAOffload();
                }
                enableFilter();
            }
        }
        eventManager_->releaseWakeLock("DataFilterDisabled");
    }
}

void NAOIpTrigger::messageReceived(IPMessage msg, int length,
    std::shared_ptr<Connection> connection) {
    LOG(DEBUG, __FUNCTION__);
    eventManager_->holdWakeLock("MessageReceived");
    TcuActivityState triggerState = TcuActivityState::UNKNOWN;
    std::string machineName = ALL_MACHINES;
    if (validateTrigger(msg.msg, length, triggerState, machineName)) {
        triggerEvent(triggerState, machineName);
    } else {
        LOG(ERROR, __FUNCTION__, " trigger not match ");
        if(eventManager_->getActivityState() == TcuActivityState::SUSPEND) {
            if(tcpKeepAliveHandler_) {
                tcpKeepAliveHandler_->stopKAOffload();
                tcpKeepAliveHandler_->startKAOffload();
            }
            enableFilter();
        }
    }
    eventManager_->releaseWakeLock("MessageReceived");
    messageCv_.notify_all();
}

bool NAOIpTrigger::loadConfig() {
    LOG(DEBUG, __FUNCTION__);
    std::map<std::string, TcuActivityState> expectedTrigger{
        {TRIGGER_SUSPEND, TcuActivityState::SUSPEND},
        {TRIGGER_RESUME, TcuActivityState::RESUME},
        {TRIGGER_SHUTDOWN, TcuActivityState::SHUTDOWN}};
    try {
        std::string configTriggerText = "";
        for (auto itr = expectedTrigger.begin(); itr != expectedTrigger.end(); ++itr) {
            configTriggerText = config_->getValue("NAOIP_TRIGGER", itr->first);
            if (!configTriggerText.empty()) {
                if (triggerText_.find(configTriggerText) != triggerText_.end()) {
                    LOG(ERROR, __FUNCTION__, " Error : same trigger for multiple state");
                    return false;
                }
                triggerText_.insert({configTriggerText, itr->second});
            }
        }
    } catch (const std::invalid_argument& ia) {
        LOG(ERROR, __FUNCTION__, " Error : invalid argument");
        return false;
    }
    return true;
}

void NAOIpTrigger::onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
    eventManager_->holdWakeLock("DataCallInfoChanged");
    if((dataCall->getDataCallStatus() == DataCallStatus::NET_CONNECTED) &&
        (eventManager_->getActivityState() == TcuActivityState::SUSPEND)) {
        for (auto connection : connectionHandler_->getConnectionList()) {
            if (dataCall->getProfileId() == connection->profileId &&
                dataCall->getSlotId() == connection->slotId) {
                if(tcpKeepAliveHandler_) {
                    tcpKeepAliveHandler_->startKAOffload();
                }
            }

            // The data filter is disabled if all data calls go down.
            // When a new data call is brought up during suspend, re-enable the data filter
            enableFilter();
        }
    }
    eventManager_->releaseWakeLock("DataCallInfoChanged");
}
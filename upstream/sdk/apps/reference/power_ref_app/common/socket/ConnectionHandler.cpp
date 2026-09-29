/*
*  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
*  SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#include "common/RefAppUtils.hpp"
#include "ConnectionHandler.hpp"
#include "TCPServer.cpp"
#include "UDPServer.cpp"
#include "TCPClient.cpp"
#include "UDPClient.cpp"
#include <algorithm>

std::shared_ptr<ConnectionHandler> ConnectionHandler::instance = nullptr;
std::once_flag ConnectionHandler::initInstanceFlag;

std::vector<std::shared_ptr<Connection>>  ConnectionHandler::getConnectionList() {
    LOG(DEBUG, __FUNCTION__);
    return connectionConfigList_;
}

ConnectionHandler::~ConnectionHandler() {
    LOG(DEBUG, __FUNCTION__);
    for (const auto &entry : connectionConfigList_) {
        entry->dataConnectionManager = nullptr;
    }
    connectionConfigList_ = {};
}

std::shared_ptr<ConnectionHandler>ConnectionHandler::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    std::call_once(initInstanceFlag, &ConnectionHandler::initSingleton);
    return instance;
}

ConnectionHandler::ConnectionHandler() {
    LOG(DEBUG, __FUNCTION__);
}

void ConnectionHandler::initSingleton() {
    LOG(DEBUG, __FUNCTION__);
    instance.reset(new ConnectionHandler());
}

bool ConnectionHandler::initialiseSocketConnection(
    std::shared_ptr<Connection> connectionConfig) {
    LOG(DEBUG, __FUNCTION__);
    if (connectionConfig->connectionRole == ConnectionRole::CLIENT) {
        if (connectionConfig->protocol == Protocol::TCP) {
            connectionConfig->socketConnection =
                std::make_shared<TCPClient>();
        } else {
            connectionConfig->socketConnection =
                std::make_shared<UDPClient>();
        }
    } else {
        if (connectionConfig->protocol == Protocol::TCP) {
            connectionConfig->socketConnection =
                std::make_shared<TCPServer>();
        } else {
            connectionConfig->socketConnection =
                std::make_shared<UDPServer>();
        }
    }
    if (!connectionConfig->socketConnection) {
        LOG(ERROR, __FUNCTION__,
            " failed to create socket connection object");
        return false;
    }
    return true;
}

// Start server/client
bool ConnectionHandler::start(std::vector<std::shared_ptr<Connection>> &connectionConfigList) {
    LOG(DEBUG, __FUNCTION__);
    {
        std::unique_lock<std::mutex> lck(mtx_);
        if (isStarted_) {
            LOG(DEBUG, __FUNCTION__, " already started");
            if(!isCompleted_) {
                cvStatusUpdate_.wait(
                    lck, [this] { return (bool)isCompleted_; });
            }
            connectionConfigList = connectionConfigList_;
            return true;
        }
        isStarted_ = true;
        isCompleted_ = false;
        isCleanupTriggered_ = false;
        connectionConfigList_ = connectionConfigList;
    }

    for (size_t i = 0; i < connectionConfigList.size(); ++i) {
        LOG(DEBUG, __FUNCTION__, " Connection: ", connectionConfigList[i]->toString());
        std::shared_ptr<Connection> connectionConfig = connectionConfigList[i];
        SlotId slotId = connectionConfig->slotId;
        connectionConfig->dataConnectionManager = initDataConnectionManager(slotId);
        if(!connectionConfig->dataConnectionManager) {
            return false;
        }
        if(!startDataCall(connectionConfig)) {
            continue;
        }

        if(!initialiseSocketConnection(connectionConfig)) {
            return false;
        }

        std::thread([this, connectionConfig] {
            do {
                {
                    std::lock_guard<std::mutex> lck(mtx_);
                    triggerReconnect_ = false;
                }

                LOG(DEBUG, __FUNCTION__, " Connection: ", connectionConfig->toString());
                if (!connectionConfig->socketConnection->isConnected()) {
                    if (!connectionConfig->socketConnection->start(connectionConfig))
                    {
                        LOG(ERROR, __FUNCTION__, " failed to start socket connection");
                    }
                }
                {
                    std::unique_lock<std::mutex> lck(mtx_);
                    cvStatusUpdate_.wait(
                        lck, [this] { return triggerReconnect_ || isCleanupTriggered_; });
                }
            } while (!isCleanupTriggered_);
        }).detach();
    }
    isCompleted_ = true;
    cvStatusUpdate_.notify_all();
    return true;
}

void ConnectionHandler::cleanup() {
    LOG(DEBUG, __FUNCTION__);
    std::unique_lock<std::mutex> lock(mtx_);
    isCleanupTriggered_ = true;
    cvStatusUpdate_.notify_all();
    for (const auto &entry : connectionConfigList_) {
        if(entry->socketConnection)
            entry->socketConnection->cleanup();
        entry->dataConnectionManager = nullptr;
    }
    connectionConfigList_ = {};
}

void ConnectionHandler::onDataCallInfoChanged(
    const std::shared_ptr<telux::data::IDataCall> &dataCall) {
    LOG(DEBUG, __FUNCTION__);
    logDataCallDetails(dataCall);

    for (const auto &entry : connectionConfigList_) {
        bool isExpectedDataCall = dataCall->getSlotId() == entry->slotId &&
            dataCall->getProfileId() == entry->profileId &&
            (   dataCall->getIpFamilyType() == telux::data::IpFamilyType::IPV4V6 ||
                dataCall->getIpFamilyType() == entry->ipFamily);

        std::unique_lock<std::mutex> lck(mtx_);
        if (isExpectedDataCall) {
            if (dataCall->getDataCallStatus() == telux::data::DataCallStatus::NET_CONNECTED) {
                entry->dataCall = dataCall;
            } else {
                entry->dataCall = nullptr;
                triggerReconnect_ = true;
            }
            cvStatusUpdate_.notify_all();
        }
    }
}

void ConnectionHandler::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " DataConnectionListener status = ",
        RefAppUtils::serviceStatusToString(status));
    bool dcmStatus = status == telux::common::ServiceStatus::SERVICE_AVAILABLE ? true : false;

    if (!dcmStatus) {
        std::unique_lock<std::mutex> lck(mtx_);
        for (const auto &entry : connectionConfigList_) {
            entry->dataCall = nullptr;
        }
        cvStatusUpdate_.notify_all();
    }
    LOG(INFO, __FUNCTION__, " isConnectionMgrReady_ = ", (int)dcmStatus);
}

std::shared_ptr<telux::data::IDataConnectionManager> ConnectionHandler::initDataConnectionManager(
    SlotId slotId) {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<telux::data::IDataConnectionManager> dataConnectionManager;
    do {
        std::promise<telux::common::ServiceStatus> dcmProm;
        // data connection mananger
        auto &dataFactory = telux::data::DataFactory::getInstance();
        dataConnectionManager = dataFactory.getDataConnectionManager(
            slotId, [&dcmProm](telux::common::ServiceStatus status) {
                dcmProm.set_value(status);
            });

        if (!dataConnectionManager) {
            LOG(ERROR, __FUNCTION__,
                " Failed to get DataConnectionManager object");
            break;
        }

        // wait for connection manager to get ready
        LOG(DEBUG, __FUNCTION__,
            " Initializing Data connection manager subsystem Please wait");
        telux::common::ServiceStatus subSystemStatus = dcmProm.get_future().get();

        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(DEBUG, __FUNCTION__, " Data Connection Manager is ready");

            if(dataConnectionManager->registerListener(
                this->shared_from_this()) != telux::common::Status::SUCCESS) {
                LOG(ERROR, __FUNCTION__,
                    " Unable to register data connection manager listener");
            }
        } else {
            LOG(ERROR, __FUNCTION__, " Data Connection Manager is failed");
        }
    } while (0);
    return dataConnectionManager;
}

void ConnectionHandler::waitForDataServiceAvailability(std::shared_ptr<Connection> connection) {
    LOG(DEBUG, __FUNCTION__);
    connection->dataServiceProvider = std::make_shared<DataServiceProvider>(connection->slotId);

    if (!connection->dataServiceProvider->init()) {
        LOG(ERROR, __FUNCTION__, "Failed to initialize DataServiceProvider");
        return;
    }
    connection->dataServiceProvider->waitForDataServiceState();
}

bool ConnectionHandler::startDataCall(std::shared_ptr<Connection> connection) {
    LOG(DEBUG, __FUNCTION__);
    telux::data::DataCallParams params;
    params.profileId = connection->profileId;
    params.ipFamilyType = connection->ipFamily;

    // Create promise and future to wait for callback
    std::promise<void> dataCallPromise;
    std::future<void> dataCallFuture = dataCallPromise.get_future();

    // Define the lambda callback
    auto dataCallCallback = [&dataCallPromise, &connection] (
        const std::shared_ptr<telux::data::IDataCall> &dataCall,
            telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            if (telux::data::DataCallStatus::NET_CONNECTED == dataCall->getDataCallStatus()) {
                LOG(DEBUG, __FUNCTION__, " start DataCallResponseCb is successful - NO_EFFECT,",
                    "data call already connected");
                connection->dataCall = dataCall;
            } else if (telux::data::DataCallStatus::NET_CONNECTING ==
                dataCall->getDataCallStatus()) {
            LOG(DEBUG, __FUNCTION__, " start DataCallResponseCb is successful");
            }
        } else {
            LOG(ERROR, __FUNCTION__, "start DataCallResponseCb failed, errorCode: ",
            static_cast<int>(error));
        }

        // Signal completion
        dataCallPromise.set_value();
    };

    // Before starting a data call, check whether the data service is ready to set up a data call
    // or not
    waitForDataServiceAvailability(connection);

    int maxAttempts = 3; // Define a maximum number of attempts
    int attempt = 0;
    while (!connection->dataCall && attempt < maxAttempts) {
        auto retStat = connection->dataConnectionManager->startDataCall(params, dataCallCallback);
        LOG(DEBUG, __FUNCTION__, " start DataCall return status: ", static_cast<int>(retStat));
        if (retStat == telux::common::Status::SUCCESS) {
            dataCallFuture.wait();
        }
        std::unique_lock<std::mutex> lck(mtx_);
        auto timeoutTime = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        if(!cvStatusUpdate_.wait_until(lck, timeoutTime, [this, connection] {
            return connection->dataCall || isCleanupTriggered_;
        })) {
            attempt++;
            LOG(DEBUG, __FUNCTION__, " Retrying startDataCall...");
        }
    }
    return connection->dataCall != nullptr;
}

void ConnectionHandler::logDataCallDetails(
    const std::shared_ptr<telux::data::IDataCall> &dataCall) {
    LOG(DEBUG, __FUNCTION__);
    std::string tmpLog;
    tmpLog =
        " ** DataCall details **\n SlotID: " +
        std::to_string((int)dataCall->getSlotId()) +
        "\n ProfileID: " + std::to_string((int)dataCall->getProfileId()) +
        "\n interfaceName: " + dataCall->getInterfaceName() +
        "\n DataCallStatus: " +
        std::to_string((int)dataCall->getDataCallStatus()) +
        "\n DataCallEndReason: Type = " +
        std::to_string(static_cast<int>(dataCall->getDataCallEndReason().type));

    LOG(DEBUG, __FUNCTION__, tmpLog);
    std::list<telux::data::IpAddrInfo> ipAddrList =
        dataCall->getIpAddressInfo();
    for (auto &it : ipAddrList) {
    tmpLog = "\n ifAddress: " + it.ifAddress +
            "\n primaryDnsAddress: " + it.primaryDnsAddress +
            "\n secondaryDnsAddress: " + it.secondaryDnsAddress;
    LOG(DEBUG, __FUNCTION__, tmpLog);
    }
    tmpLog = " IpFamilyType: " +
            std::to_string(static_cast<int>(dataCall->getIpFamilyType())) +
            "\nTechPreference: " +
            std::to_string(static_cast<int>(dataCall->getTechPreference())) +
            "\n DataBearerTechnology: " +
            std::to_string(static_cast<int>(dataCall->getCurrentBearerTech()));
    LOG(DEBUG, __FUNCTION__, tmpLog);
}

DataServiceProvider::DataServiceProvider(SlotId slotId) : slotId_(slotId), inService_(false) {
    LOG(DEBUG, __FUNCTION__);
}

void DataServiceProvider::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " ServiceStatus: ", static_cast<int>(status));
}

void DataServiceProvider::onServiceStateChanged(telux::data::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (status.serviceState == telux::data::DataServiceState::IN_SERVICE) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            inService_ = true;
        }
        cvInService_.notify_one();
    } else {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            inService_ = false;
        }
    }
}

void DataServiceProvider::waitForDataServiceState() {
    LOG(DEBUG, __FUNCTION__);
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    bool callbackExecuted = false;

    auto callback = [this, &promise, &callbackExecuted](
        telux::data::ServiceStatus status, telux::common::ErrorCode errorCode) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callbackExecuted = true;
            if(status.serviceState == telux::data::DataServiceState::IN_SERVICE) {
                inService_ = true;
            } else {
                inService_ = false;
            }
        }
        promise.set_value();
    };

    auto status = servingSystemManager_->requestServiceStatus(callback);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "requestServiceStatus failed with status: ",
            static_cast<int>(status));
        return;
    }

    future.wait();

    std::unique_lock<std::mutex> lock(mutex_);

    if (!inService_) {
        // If inService_ is not set, wait for onServiceStateChanged to be called
        cvInService_.wait(lock, [this] { return inService_; });
    }
}

bool DataServiceProvider::init() {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<telux::data::IServingSystemManager> dataServingSystemManager;
    do {
        std::promise<telux::common::ServiceStatus> dcmProm;
        // data serving system mananger
        auto &dataFactory = telux::data::DataFactory::getInstance();
        dataServingSystemManager = dataFactory.getServingSystemManager(
            slotId_, [&dcmProm](telux::common::ServiceStatus status) {
                dcmProm.set_value(status);
            });

        if (!dataServingSystemManager) {
            LOG(ERROR, __FUNCTION__,
                " Failed to get dataServingSystemManager object");
            return false;
        }

        // wait for serving system manager to get ready
        LOG(DEBUG, __FUNCTION__,
            " Initializing dataServingSystemManager subsystem Please wait");
        telux::common::ServiceStatus subSystemStatus = dcmProm.get_future().get();

        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(DEBUG, __FUNCTION__, " dataServingSystemManager is ready");
            if(dataServingSystemManager->registerListener(
                this->shared_from_this()) != telux::common::Status::SUCCESS) {
                LOG(ERROR, __FUNCTION__,
                    " Unable to register dataServingSystemManager listener");
            }
        } else {
            LOG(ERROR, __FUNCTION__, " dataServingSystemManager is failed");
        }
    } while (0);
    servingSystemManager_ = dataServingSystemManager;
    return true;
}
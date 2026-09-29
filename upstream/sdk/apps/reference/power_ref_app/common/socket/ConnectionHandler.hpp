/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CONNECTION_HANDLER_HPP
#define CONNECTION_HANDLER_HPP

#include <memory>
#include <mutex>
#include <telux/data/DataFactory.hpp>
#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataDefines.hpp>
#include <vector>

#define DEFAULT_PROFILE 1

class DataServiceProvider
    : public telux::data::IServingSystemListener,
        public std::enable_shared_from_this<DataServiceProvider> {

public:
    DataServiceProvider(SlotId slotId);
    bool init();
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onServiceStateChanged(telux::data::ServiceStatus status) override;
    void waitForDataServiceState();
private:

    SlotId slotId_;
    std::shared_ptr<telux::data::IServingSystemManager> servingSystemManager_;
    std::mutex mutex_;
    std::condition_variable cvInService_;
    bool inService_;
};

class ConnectionHandler
    : public telux::data::IDataConnectionListener,
      public std::enable_shared_from_this<ConnectionHandler> {

public:
    ConnectionHandler(const ConnectionHandler &) = delete;
    ConnectionHandler &operator=(const ConnectionHandler &) = delete;

    // Start server/client
    bool start(std::vector<std::shared_ptr<Connection>> &connectionConfigList);

    void cleanup();

    void onDataCallInfoChanged(
        const std::shared_ptr<telux::data::IDataCall> &dataCall) override;

    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    std::vector<std::shared_ptr<Connection>>  getConnectionList();

    ~ConnectionHandler();

    static std::shared_ptr<ConnectionHandler> getInstance();

    private:
    ConnectionHandler();

    static void initSingleton();

    std::shared_ptr<telux::data::IDataConnectionManager> initDataConnectionManager(SlotId slotId);
    void waitForDataServiceAvailability(std::shared_ptr<Connection> connection);
    bool initialiseSocketConnection(std::shared_ptr<Connection> connection);

    bool startDataCall(std::shared_ptr<Connection> connection);

    void logDataCallDetails(const std::shared_ptr<telux::data::IDataCall> &dataCall);

    std::atomic<bool> receivedStopClient_ = {false};

    std::mutex mtx_;
    std::condition_variable cvStatusUpdate_;
    std::atomic<bool> isStarted_ = {false};
    std::atomic<bool> isCompleted_ = {false};
    std::atomic<bool> isCleanupTriggered_ = {false};
    std::atomic<bool> triggerReconnect_ = {false};
    std::vector<std::shared_ptr<Connection>> connectionConfigList_;
    static std::shared_ptr<ConnectionHandler> instance;
    static std::once_flag initInstanceFlag;
};

#endif // CONNECTION_HANDLER_HPP

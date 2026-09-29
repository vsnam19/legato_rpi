/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "./TCPKeepAliveHandler.hpp"
#include <future>

std::shared_ptr<TCPKeepAliveHandler>
TCPKeepAliveHandler::getInstance(std::shared_ptr<EventManager> eventManager) {
  static std::shared_ptr<TCPKeepAliveHandler> instance(
      new TCPKeepAliveHandler(eventManager));
  return instance;
}

// Private constructor
TCPKeepAliveHandler::TCPKeepAliveHandler(
    std::shared_ptr<EventManager> eventManager)
    : eventManager_(eventManager) {}

TCPKeepAliveHandler::~TCPKeepAliveHandler() {
  if(connectionHandler_) {
    connectionHandler_->cleanup();
    connectionHandler_ = nullptr;
  }
  connectionKaInfoList_ = {};
  eventManager_= nullptr;
}

bool TCPKeepAliveHandler::init() {
    // Connection handler initialisation
  connectionHandler_ = ConnectionHandler::getInstance();
  if (!connectionHandler_) {
    return false;
  }
  std::shared_ptr<ISocketConnectionListener> listener = shared_from_this();

  std::vector<std::shared_ptr<Connection>> connectionList =  RefAppUtils::getConnectionConfigs();
  if(!connectionHandler_->start(connectionList)) {
    LOG(DEBUG, __FUNCTION__, " Connection handler is failed");
    return false;
  }

  auto &dataFactory = telux::data::DataFactory::getInstance();
  for (auto connection : connectionList) {
    std::shared_ptr<ConnectionKaInfo> connectionKaInfo = make_shared<ConnectionKaInfo>();
    connectionKaInfo->connection = connection;
    if(connection->socketConnection != nullptr) {
      connection->socketConnection->registerListener(listener);
    } else {
      LOG(DEBUG, __FUNCTION__, " socket connection failed");
      return false;
    }

    // Keep alive manager initialisation
    std::promise<telux::common::ServiceStatus> kaProm;
    std::shared_ptr<telux::data::IKeepAliveManager> keepAliveManager =
      dataFactory.getKeepAliveManager(connection->slotId,
      [&kaProm](telux::common::ServiceStatus status) {
        LOG(DEBUG, __FUNCTION__, " Callback invoked ",
            static_cast<int>(status));
        kaProm.set_value(status);
      });

    if (!keepAliveManager) {
      LOG(DEBUG, __FUNCTION__, " Failed to get keepAliveMgr object");
      return false;
    }

    LOG(DEBUG, __FUNCTION__, " Initializing keep alive subsystem Please wait");
    telux::common::ServiceStatus subSystemStatus = kaProm.get_future().get();
    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      LOG(DEBUG, __FUNCTION__, " Keep alive Manager is ready");
    } else {
      LOG(DEBUG, __FUNCTION__, " Keep alive Manager is failed");
      keepAliveManager = nullptr;
      return false;
    }
    connectionKaInfo->keepAliveManager = keepAliveManager;
    connectionKaInfoList_.push_back(connectionKaInfo);
  }
  return true;
}

void TCPKeepAliveHandler::onConnect(std::shared_ptr<Connection> connection) {
  LOG(DEBUG, __FUNCTION__, connection->toString());
}

void TCPKeepAliveHandler::messageReceived(IPMessage msg,
  int length, std::shared_ptr<Connection> connection) {
  std::string msgString = std::string(msg.msg, length);
  LOG(DEBUG, __FUNCTION__, connection->toString(), "\n message: ", msgString);
}

void TCPKeepAliveHandler::onDisconnect(std::shared_ptr<Connection> connection) {
  LOG(DEBUG, __FUNCTION__, connection->toString());
}

void TCPKeepAliveHandler::onEventRejected(shared_ptr<Event> event,
                                          EventStatus reason) {}

bool TCPKeepAliveHandler::startKAOffload() {
  LOG(DEBUG, __FUNCTION__, "connection list size: ", connectionKaInfoList_.size());


  for (auto connectionKaInfo : connectionKaInfoList_) {
    if (connectionKaInfo->monitorHandle && connectionKaInfo->offloadHandle) {
      LOG(DEBUG, __FUNCTION__, " KA offload already started");
      continue;
    }
    if (!(connectionKaInfo->connection && connectionKaInfo->connection->socketConnection &&
      connectionKaInfo->connection->socketConnection->isConnected())) {
      LOG(DEBUG, __FUNCTION__, " connection not connected");
      continue;
    }
    LOG(DEBUG, __FUNCTION__, connectionKaInfo->connection->toString());
    if ((!connectionKaInfo->connection->serverIpAddr.empty()) &&
        (!connectionKaInfo->connection->clientIpAddr.empty())) {
      telux::data::TCPKAParams kaPram = {};
      if (connectionKaInfo->connection->connectionRole == ConnectionRole::CLIENT) {
        kaPram.srcIp = connectionKaInfo->connection->clientIpAddr;
        kaPram.dstIp = connectionKaInfo->connection->serverIpAddr;
        kaPram.srcPort = connectionKaInfo->connection->clientPort;
        kaPram.dstPort = connectionKaInfo->connection->serverPort;
      } else {
        kaPram.dstIp = connectionKaInfo->connection->clientIpAddr;
        kaPram.srcIp = connectionKaInfo->connection->serverIpAddr;
        kaPram.dstPort = connectionKaInfo->connection->clientPort;
        kaPram.srcPort = connectionKaInfo->connection->serverPort;
      }

      if (connectionKaInfo->keepAliveManager->enableTCPMonitor(
        kaPram, connectionKaInfo->monitorHandle) == telux::common::ErrorCode::SUCCESS) {
        IPMessage msg;
        memset(&msg, 0, sizeof(msg));
        const char *message = "Hello\n";
        std::copy(message, message + strlen(message) + 1, msg.msg);
        connectionKaInfo->connection->socketConnection->sendMessage(msg);
        connectionKaInfo->connection->socketConnection->ensureAllPacketsAcknowledged();
        // std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (connectionKaInfo->keepAliveManager->startTCPKeepAliveOffload(
                connectionKaInfo->monitorHandle, RefAppUtils::getKeepAliveInterval(),
                connectionKaInfo->offloadHandle) == telux::common::ErrorCode::SUCCESS) {
          continue;
        } else {
          LOG(ERROR, __FUNCTION__, " issue in startTCPKeepAliveOffload");
        }
      } else {
        LOG(ERROR, __FUNCTION__, " issue in enableTCPMonitor");
      }
    } else {
      LOG(ERROR, __FUNCTION__, " issue in connection");
    }
  }
  return false;
}

void TCPKeepAliveHandler::stopKAOffload() {
  for (auto connectionKaInfo : connectionKaInfoList_) {
    if (connectionKaInfo->offloadHandle) {
      connectionKaInfo->keepAliveManager->stopTCPKeepAliveOffload(connectionKaInfo->offloadHandle);
      connectionKaInfo->offloadHandle = 0;
    }
    if (connectionKaInfo->monitorHandle) {
      connectionKaInfo->keepAliveManager->disableTCPMonitor(connectionKaInfo->monitorHandle);
      connectionKaInfo->monitorHandle = 0;
    }
  }
}

void TCPKeepAliveHandler::onEventProcessed(shared_ptr<Event> event,
                                           bool success) {
  if (success) {
    if (event->getTriggeredState() == telux::power::TcuActivityState::SUSPEND) {
      startKAOffload();
    } else if (event->getTriggeredState() ==
               telux::power::TcuActivityState::RESUME) {
      stopKAOffload();
    }
  }
}

void TCPKeepAliveHandler::preProcessEvent(shared_ptr<Event> event) {}

void TCPKeepAliveHandler::onKeepAliveStatusChange(
    telux::common::ErrorCode error, telux::data::TCPKAOffloadHandle handle) {
  if (error != telux::common::ErrorCode::SUCCESS) {
    if (error == ErrorCode::NETWORK_ERR) {
      LOG(ERROR, __FUNCTION__, "TCP keep-alive offloading error NETWORK_ERR.");
    } else if (error == ErrorCode::CANCELLED) {
      LOG(ERROR, __FUNCTION__,
          "TCP keep-alive offloading error ErrorCode::CANCELLED.");
    } else {
      LOG(ERROR, __FUNCTION__,
          "TCP keep-alive offloading error : ", static_cast<int>(error));
    }
  }
}

void TCPKeepAliveHandler::onServiceStatusChange(
    telux::common::ServiceStatus status) {
  LOG(ERROR, __FUNCTION__, " keep alive manager status: ", static_cast<int>(status));
}

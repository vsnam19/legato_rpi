/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TCP_KEEP_ALIVE_HANDLER_HPP
#define TCP_KEEP_ALIVE_HANDLER_HPP

#include <telux/common/Log.hpp>
#include <telux/data/KeepAliveManager.hpp>

#include "EventManager.hpp"
#include "common/socket/ConnectionHandler.hpp"

struct ConnectionKaInfo {
  std::shared_ptr<Connection> connection;
  std::shared_ptr<telux::data::IKeepAliveManager> keepAliveManager;
  telux::data::MonitorHandleType monitorHandle = 0;
  telux::data::TCPKAOffloadHandle offloadHandle = 0;
};

class TCPKeepAliveHandler
    : public ISocketConnectionListener,
      public IEventListener,
      public telux::data::IKeepAliveListener,
      public enable_shared_from_this<TCPKeepAliveHandler> {

public:
  static std::shared_ptr<TCPKeepAliveHandler>
  getInstance(std::shared_ptr<EventManager> eventManager);

  TCPKeepAliveHandler(const TCPKeepAliveHandler &) = delete;
  TCPKeepAliveHandler &operator=(const TCPKeepAliveHandler &) = delete;

  ~TCPKeepAliveHandler();
  bool init();
  bool startKAOffload();
  void stopKAOffload();

  void onConnect(std::shared_ptr<Connection> connection) override;
  void messageReceived(IPMessage msg, int length, std::shared_ptr<Connection> connection) override;
  void onDisconnect(std::shared_ptr<Connection> connection) override;

  void onEventRejected(shared_ptr<Event> event, EventStatus reason) override;
  void onEventProcessed(shared_ptr<Event> event, bool success) override;
  void preProcessEvent(shared_ptr<Event> event) override;

  void onKeepAliveStatusChange(telux::common::ErrorCode error,
                               telux::data::TCPKAOffloadHandle handle) override;

  void onServiceStatusChange(telux::common::ServiceStatus status) override;

private:
  TCPKeepAliveHandler(std::shared_ptr<EventManager> eventManager);

  std::shared_ptr<ConnectionHandler> connectionHandler_;
  std::vector<std::shared_ptr<ConnectionKaInfo>> connectionKaInfoList_;
  std::shared_ptr<EventManager> eventManager_;
};

#endif // TCP_KEEP_ALIVE_HANDLER_HPP
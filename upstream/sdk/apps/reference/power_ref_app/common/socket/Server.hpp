/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef ISERVER_HPP
#define ISERVER_HPP

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <telux/data/DataDefines.hpp>
#include <unistd.h>
#include <netinet/tcp.h>
#include "ISocketConnectionListener.hpp"
#include <telux/common/Log.hpp>

class IServer : public IIPConnection {
public:
  IServer() {}
  virtual ~IServer() {}

  virtual bool isStarted() override = 0;
  virtual bool isConnected() override = 0;
  virtual std::shared_ptr<Connection> getConnectionParams() override = 0;
  virtual bool start(std::shared_ptr<Connection> connectionConfig) override = 0;
  virtual bool sendMessage(IPMessage &msg) override = 0;
  virtual void cleanup() override = 0;
  virtual bool ensureAllPacketsAcknowledged() override = 0;

  virtual void registerListener(std::shared_ptr<ISocketConnectionListener> listener) override {
    LOG(DEBUG, __FUNCTION__);
    listeners_.push_back(listener);
  }

protected:
  std::vector<std::shared_ptr<ISocketConnectionListener>> listeners_;
  std::shared_ptr<Connection> connectionConfig_;
  std::atomic<bool> isConnected_ = {false};
  std::atomic<bool> isReceivedStopServer_ = {false};
};

#endif // ISERVER_HPP

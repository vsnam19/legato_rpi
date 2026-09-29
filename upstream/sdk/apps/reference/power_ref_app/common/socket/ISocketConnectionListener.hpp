/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ISOCKET_CONNECTION_LISTENER_HPP
#define ISOCKET_CONNECTION_LISTENER_HPP

#include <string.h>
#include <telux/data/DataDefines.hpp>
#include <sstream>
#include <telux/data/DataConnectionManager.hpp>

class IIPConnection;
class ISocketConnectionListener;
class DataServiceProvider;

enum class ConnectionRole { CLIENT = 0, SERVER = 1 };

enum class Protocol { TCP = 6, UDP = 17 };

/**
 * Content/formate of IP message
 */
struct IPMessage {
  char msg[1024];
};

struct Connection {
  Protocol protocol;
  telux::data::IpFamilyType ipFamily;
  ConnectionRole connectionRole;
  std::string serverIpAddr = "";
  std::string clientIpAddr = "";
  int serverPort = 0;
  int clientPort = 0;

  SlotId slotId;
  int profileId;

  std::shared_ptr<telux::data::IDataCall> dataCall = nullptr;
  std::shared_ptr<IIPConnection> socketConnection = nullptr;
  std::shared_ptr<telux::data::IDataConnectionManager> dataConnectionManager = nullptr;
  std::shared_ptr<DataServiceProvider> dataServiceProvider = nullptr;

  static std::string enumToString(Protocol val) {
    switch (val) {
      case Protocol::TCP: return "TCP";
      case Protocol::UDP: return "UDP";
      default: return "Unknown Protocol";
    }
  }

  static std::string enumToString(telux::data::IpFamilyType val) {
    switch (val) {
      case telux::data::IpFamilyType::UNKNOWN: return "UNKNOWN";
      case telux::data::IpFamilyType::IPV4: return "IPV4";
      case telux::data::IpFamilyType::IPV6: return "IPV6";
      case telux::data::IpFamilyType::IPV4V6: return "IPV4V6";
      default: return "Unknown IP Family";
    }
  }

  static std::string enumToString(ConnectionRole val) {
    switch (val) {
      case ConnectionRole::CLIENT: return "CLIENT";
      case ConnectionRole::SERVER: return "SERVER";
      default: return "Unknown Role";
    }
  }

  std::string toString() const {
    std::ostringstream oss;
    oss << "Protocol: " << enumToString(protocol)
        << ", IP Family: " << enumToString(ipFamily)
        << ", Connection Role: " << enumToString(connectionRole)
        << ", Server IP: " << serverIpAddr
        << ", Client IP: " << clientIpAddr
        << ", Server Port: " << serverPort
        << ", Client Port: " << clientPort;
    return oss.str();
  }
};

class IIPConnection {
public:
  virtual bool isStarted() = 0;
  virtual bool start(std::shared_ptr<Connection> connectionConfigList) = 0;
  virtual bool isConnected() = 0;
  virtual bool sendMessage(IPMessage &msg) = 0;
  virtual std::shared_ptr<Connection> getConnectionParams() = 0;
  virtual void cleanup() = 0;
  virtual bool ensureAllPacketsAcknowledged() = 0;
  virtual void registerListener(std::shared_ptr<ISocketConnectionListener> listener) = 0;
};

class ISocketConnectionListener {
public:
  virtual void onConnect(std::shared_ptr<Connection> connection) {}
  virtual void messageReceived(IPMessage msg, int length,
    std::shared_ptr<Connection> connection) {}
  virtual void onDisconnect(std::shared_ptr<Connection> connection) {}
};


#endif // ISOCKET_CONNECTION_LISTENER_HPP
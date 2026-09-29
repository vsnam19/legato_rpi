/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef UDP_CLIENT_CPP
#define UDP_CLIENT_CPP

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <telux/data/DataDefines.hpp>
#include <unistd.h>
#include <vector>
#include "ISocketConnectionListener.hpp"
#include <telux/common/Log.hpp>

class UDPClient : public IIPConnection {

public:
  UDPClient() {
    LOG(DEBUG, __FUNCTION__);
  }
  ~UDPClient() {
    LOG(DEBUG, __FUNCTION__);
    cleanup();
  }

  bool isStarted() override {
    LOG(DEBUG, __FUNCTION__);
    return !receivedStopClient_;
  }

  bool isConnected() override {
    LOG(DEBUG, __FUNCTION__);
    return isConnected_;
  }

  std::shared_ptr<Connection> getConnectionParams() override {
    LOG(DEBUG, __FUNCTION__);
    std::unique_lock<std::mutex> lock(mtx_);
    return connectionConfig_;
  }

  bool bindToDevice(int clientSocket, std::string deviceName) {
      LOG(DEBUG, __FUNCTION__);
      if (setsockopt(clientSocket, SOL_SOCKET, SO_BINDTODEVICE,
          deviceName.c_str(), deviceName.size()) != 0) {
          LOG(ERROR, __FUNCTION__, "Failed to bind to device: ", strerror(errno));
          return false;
      }
      return true;
  }

  bool updateConnectionParams() {
    LOG(DEBUG, __FUNCTION__);
    char ip_str[INET6_ADDRSTRLEN];
    uint16_t port;

    // Get the local (source) address and port
    struct sockaddr_storage local_addr;
    socklen_t addr_len = sizeof(local_addr);
    if (getsockname(clientSocket_, (struct sockaddr *)&local_addr, &addr_len) <
        0) {
      LOG(ERROR, __FUNCTION__, "Error getting local address");
      close(clientSocket_);
      isConnected_ = false;
      return false;
    }
    if (local_addr.ss_family == AF_INET) { // IPv4
      struct sockaddr_in *addr_in = (struct sockaddr_in *)&local_addr;
      inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, sizeof(ip_str));
      port = ntohs(addr_in->sin_port);
      connectionConfig_->clientIpAddr = ip_str;
      connectionConfig_->clientPort = port;
      connectionConfig_->ipFamily = telux::data::IpFamilyType::IPV4;
    } else if (local_addr.ss_family == AF_INET6) { // IPv6
      struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&local_addr;
      inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ip_str, sizeof(ip_str));
      port = ntohs(addr_in6->sin6_port);
      connectionConfig_->clientIpAddr = ip_str;
      connectionConfig_->clientPort = port;
      connectionConfig_->ipFamily = telux::data::IpFamilyType::IPV6;
    }

    // Get the remote (destination) address and port
    struct sockaddr_storage remote_addr;
    addr_len = sizeof(remote_addr);
    if (getpeername(clientSocket_, (struct sockaddr *)&remote_addr, &addr_len) <
        0) {
      LOG(ERROR, __FUNCTION__, "Error getting remote address");
      close(clientSocket_);
      isConnected_ = false;
      return false;
    }
    if (remote_addr.ss_family == AF_INET) { // IPv4
      struct sockaddr_in *addr_in = (struct sockaddr_in *)&remote_addr;
      inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, sizeof(ip_str));
      port = ntohs(addr_in->sin_port);
      connectionConfig_->serverIpAddr = ip_str;
      connectionConfig_->serverPort = port;
    } else if (remote_addr.ss_family == AF_INET6) { // IPv6
      struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&remote_addr;
      inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ip_str, sizeof(ip_str));
      port = ntohs(addr_in6->sin6_port);
      connectionConfig_->serverIpAddr = ip_str;
      connectionConfig_->serverPort = port;
    }
    connectionConfig_->protocol = connectionConfig_->protocol;
    // Call the worker onConnect method with the updated connection info
    for (auto listener : listeners_) {
      listener->onConnect(connectionConfig_);
    }

    LOG(DEBUG, __FUNCTION__, connectionConfig_->toString());
    return true;
  }
  bool readLoop() {
    LOG(DEBUG, __FUNCTION__);
    if (clientSocket_ < 0) {
      LOG(ERROR, __FUNCTION__, " Invalid socket descriptor");
      return false;
    }

    try {
      do {
        IPMessage msg;
        memset(&msg, 0, sizeof(msg));
        ssize_t n = 0;

        // Since UDP is connectionless, send a sample message to help the server detect the client
        IPMessage sampleMsg;
        memset(&sampleMsg, 0, sizeof(sampleMsg));
        const char *message = "\n";
        std::copy(message, message + strlen(message) + 1, sampleMsg.msg);
        sendMessage(sampleMsg);

        n = recv(clientSocket_, static_cast<void *>(&msg), sizeof(msg), 0);
        if (n <= 0) {
          LOG(ERROR, __FUNCTION__, " connection interrupted or closed");
          isConnected_ = false;
          break;
        }

        LOG(DEBUG, __FUNCTION__, " length = ", n);
        for (auto listener : listeners_) {
          listener->messageReceived(msg, n, connectionConfig_);
        }

      } while (true);

      for (auto listener : listeners_) {
        listener->onDisconnect(connectionConfig_);
      }

      if (close(clientSocket_) == -1) {
        LOG(ERROR, __FUNCTION__,
            "close failed errno = ", std::string(strerror(errno)));
      }

    } catch (const std::exception &e) {
      isConnected_ = false;
      LOG(ERROR, __FUNCTION__, " exception: ", std::string(e.what()));
    }

    LOG(DEBUG, __FUNCTION__, " exit ");
    return true;
  }

  bool start(std::shared_ptr<Connection> connectionConfig) override {
    LOG(DEBUG, __FUNCTION__);
    receivedStopClient_ = false;
    connectionConfig_ = connectionConfig;
    std::lock_guard<std::mutex> lk(mtx_);
    if (isConnected_) {
      return false;
    }

    startUdp();
    return true;
  }

  void startUdp() {
    std::thread([=]() {
      isConnected_ = false;
      do {
        LOG(DEBUG, __FUNCTION__, " UDP client starting...");

        if (!setupSocketAndBind())
          return;

        if (!connectToServer()) {
          usleep(4000000); // Retry delay
          continue;
        }
        isConnected_ = true;
        readLoop(); // Ensure this uses recvfrom/sendto for UDP
      } while (1);
    }).detach();

  }

  bool setupSocketAndBind() {
    LOG(DEBUG, __FUNCTION__);
    int domain = (connectionConfig_->ipFamily == telux::data::IpFamilyType::IPV6)
                     ? AF_INET6
                     : AF_INET;
    clientSocket_ = socket(domain, SOCK_DGRAM, 0);
    if (clientSocket_ < 0) {
      LOG(ERROR, __FUNCTION__, " socket : ", std::string(strerror(errno)));
      return false;
    }

    if(!bindToDevice(clientSocket_, connectionConfig_->dataCall->getInterfaceName())) {
      return false;
    }

    struct sockaddr *sockAddrBind = nullptr;
    socklen_t sockSize = 0;
    int reuse = 1;

    // Prepare bind address
    if (!prepareBindAddress(&sockAddrBind, &sockSize)) {
      return false;
    }

    if (sockAddrBind != nullptr) {
      setsockopt(clientSocket_, SOL_SOCKET, SO_REUSEADDR, &reuse,
                 sizeof(reuse));
      setsockopt(clientSocket_, SOL_SOCKET, SO_REUSEPORT, &reuse,
                 sizeof(reuse));
      if (bind(clientSocket_, sockAddrBind, sockSize) < 0) {
        LOG(ERROR, __FUNCTION__, " bind : ", std::string(strerror(errno)));
        return false;
      }
    }
    return true;
  }

  bool connectToServer() {
    LOG(DEBUG, __FUNCTION__);
    struct sockaddr *sockAddrConnect = nullptr;
    socklen_t sockSize = 0;

    if (connectionConfig_->ipFamily == telux::data::IpFamilyType::IPV4) {
      struct sockaddr_in v4ServerAddr = {};
      if (!inet_pton(AF_INET, connectionConfig_->serverIpAddr.c_str(),
                     &(v4ServerAddr.sin_addr))) {
        LOG(ERROR, __FUNCTION__, " failed destination IPv4 parsing");
        return false;
      }
      v4ServerAddr.sin_family = AF_INET;
      v4ServerAddr.sin_port = htons(connectionConfig_->serverPort);
      sockAddrConnect = reinterpret_cast<struct sockaddr *>(&v4ServerAddr);
      sockSize = sizeof(sockaddr_in);
    } else {
      struct sockaddr_in6 v6ServerAddr = {};
      if (!inet_pton(AF_INET6, connectionConfig_->serverIpAddr.c_str(),
                     &(v6ServerAddr.sin6_addr))) {
        LOG(ERROR, __FUNCTION__, " failed destination IPv6 parsing");
        return false;
      }
      v6ServerAddr.sin6_family = AF_INET6;
      v6ServerAddr.sin6_port = htons(connectionConfig_->serverPort);
      sockAddrConnect = reinterpret_cast<struct sockaddr *>(&v6ServerAddr);
      sockSize = sizeof(sockaddr_in6);
    }

    if (connect(clientSocket_, sockAddrConnect, sockSize) == -1) {
      LOG(ERROR, __FUNCTION__, " connect : ", std::string(strerror(errno)), "; Connection config: ",
        connectionConfig_->toString());
      return false;
    }

    return true;
  }

  bool prepareBindAddress(struct sockaddr **sockAddrBind, socklen_t *sockSize) {
    if (connectionConfig_->ipFamily == telux::data::IpFamilyType::IPV4) {
      static struct sockaddr_in v4ClientAddr = {};
      if (!connectionConfig_->dataCall->getIpv4Info().addr.ifAddress.empty()) {
        if (!inet_pton(AF_INET,
            this->connectionConfig_->dataCall->getIpv4Info().addr.ifAddress.c_str(),
              &(v4ClientAddr.sin_addr))) {
          LOG(ERROR, __FUNCTION__, " failed source IPv4 parsing ",
              std::string(strerror(errno)));
          return false;
        }
      }
      v4ClientAddr.sin_family = AF_INET;
      if (connectionConfig_->clientPort != 0) {
        v4ClientAddr.sin_port = htons(connectionConfig_->clientPort);
      }
      *sockAddrBind = reinterpret_cast<struct sockaddr *>(&v4ClientAddr);
      *sockSize = sizeof(sockaddr_in);
    } else {
      static struct sockaddr_in6 v6ClientAddr = {};
      if (!connectionConfig_->dataCall->getIpv6Info().addr.ifAddress.empty()) {
        if (!inet_pton(AF_INET6, connectionConfig_->dataCall->getIpv6Info().addr.ifAddress.c_str(),
                       &(v6ClientAddr.sin6_addr))) {
          LOG(ERROR, __FUNCTION__, " failed source IPv6 parsing ",
              std::string(strerror(errno)));
          return false;
        }
      }
      v6ClientAddr.sin6_family = AF_INET6;
      if (connectionConfig_->clientPort != 0) {
        v6ClientAddr.sin6_port = htons(connectionConfig_->clientPort);
      }
      *sockAddrBind = reinterpret_cast<struct sockaddr *>(&v6ClientAddr);
      *sockSize = sizeof(sockaddr_in6);
    }
    return true;
  }

  void cleanup() override {
    LOG(ERROR, __FUNCTION__, " Stopping  client ");
    receivedStopClient_ = true;
    isConnected_ = false;
    if (clientSocket_ != -1) {
      if (close(clientSocket_) == -1) {
        LOG(ERROR, __FUNCTION__, " close : ", std::string(strerror(errno)));
      }
    }
    clientSocket_ = -1;
  }

  bool sendMessage(IPMessage &msg) override {
    LOG(DEBUG, __FUNCTION__);
    if (send(clientSocket_, static_cast<const void *>(&msg), sizeof(IPMessage), 0) !=
        sizeof(IPMessage)) {
      LOG(ERROR, __FUNCTION__, " send : ", std::string(strerror(errno)));
      for (auto listener : listeners_) {
        listener->onDisconnect(connectionConfig_);
      }
      close(clientSocket_);
      isConnected_ = false;
      return false;
    }
    return true;
  }

  bool ensureAllPacketsAcknowledged() {
    return true;
  }

  void registerListener(std::shared_ptr<ISocketConnectionListener> listener) override {
    listeners_.push_back(listener);
  }

private:
  std::vector<std::shared_ptr<ISocketConnectionListener>> listeners_;
  std::shared_ptr<Connection> connectionConfig_;
  std::thread clientThread_;
  int clientSocket_;
  std::atomic<bool> receivedStopClient_ = {false};
  std::mutex mtx_;
  std::atomic<bool> isConnected_ = {false};
};
#endif // UDP_CLIENT_CPP

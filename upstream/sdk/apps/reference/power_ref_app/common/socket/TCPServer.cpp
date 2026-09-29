/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#include "Server.hpp"

class TCPServer : public IServer {
public:
  TCPServer() {
    LOG(DEBUG, __FUNCTION__);
  }
  ~TCPServer() {
    LOG(DEBUG, __FUNCTION__);
    cleanup();
  }

  bool isStarted() override {
    LOG(DEBUG, __FUNCTION__);
    return !this->isReceivedStopServer_;
  }

  bool isConnected() override {
    LOG(DEBUG, __FUNCTION__);
    return this->isConnected_;
  }

  std::shared_ptr<Connection> getConnectionParams() override {
    LOG(DEBUG, __FUNCTION__);
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

  bool start(std::shared_ptr<Connection> connectionConfig) override {
    LOG(DEBUG, __FUNCTION__);
    connectionConfig_ = connectionConfig;
    isReceivedStopServer_ = false;
    int domain = connectionConfig->ipFamily == telux::data::IpFamilyType::IPV4
                     ? AF_INET
                     : AF_INET6;
    serverSocket_ = socket(domain, SOCK_STREAM, 0);
    if (serverSocket_ == -1) {
      LOG(ERROR, __FUNCTION__, " socket : ", std::string(strerror(errno)));
      this->isConnected_ = false;
      return false;
    }
    // Bind and listen
    if (!bindSocket()) {
      return false;
    }
    std::thread([this]() { listenTCPSync(); }).detach();
    return true;
  }

  bool sendMessage(IPMessage &msg) override {
    LOG(DEBUG, __FUNCTION__);
    if (send(clientSocket_, static_cast<const void *>(&msg), sizeof(IPMessage), 0) !=
        sizeof(IPMessage)) {
      LOG(ERROR, __FUNCTION__, " send : ", std::string(strerror(errno)));
      this->isConnected_ = false;
      for (auto listener : listeners_) {
        listener->onDisconnect(connectionConfig_);
      }
      close(clientSocket_);
      return false;
    }
    return true;
  }

  bool ensureAllPacketsAcknowledged() override {
    LOG(DEBUG, __FUNCTION__);
    if (connectionConfig_->protocol == Protocol::TCP) {
      while (true) {
        struct tcp_info info;
        socklen_t len = sizeof(info);
        if (getsockopt(clientSocket_, IPPROTO_TCP, TCP_INFO, &info, &len) ==
            0) {
          if (info.tcpi_unacked == 0) {
            break; // All ACKs received
          }
        } else {
          LOG(ERROR, __FUNCTION__,
              " getsockopt : ", std::string(strerror(errno)));
          return false;
        }
        usleep(100000); // Sleep 100ms to avoid busy-waiting
      }
    }
    return true;
  }

  void cleanup() override {
    LOG(DEBUG, __FUNCTION__, " Stopping TCP Server ");
    this->isReceivedStopServer_ = true;
    this->isConnected_ = false;
    if (serverSocket_ != -1) {
      if (shutdown(serverSocket_, SHUT_RDWR) == -1) {
        LOG(ERROR, __FUNCTION__, " shutdown ", std::string(strerror(errno)));
      }
      if (close(serverSocket_) == -1) {
        LOG(ERROR, __FUNCTION__, " close ", std::string(strerror(errno)));
      }
    }
    if (clientSocket_ != -1) {
      if (shutdown(clientSocket_, SHUT_RDWR) == -1) {
        LOG(ERROR, __FUNCTION__, "client shutdown ",
            std::string(strerror(errno)));
      }
      if (close(clientSocket_) == -1) {
        LOG(ERROR, __FUNCTION__, "client close ", std::string(strerror(errno)));
      }
    }
    serverSocket_ = -1;
    clientSocket_ = -1;
  }

private:
  int serverSocket_ = -1;
  int clientSocket_ = -1;

  bool bindSocket() {
    LOG(DEBUG, __FUNCTION__);
    struct sockaddr *sockAddr = nullptr;
    socklen_t sockSize = 0;
    int reuse = 1;

    if(!bindToDevice(serverSocket_, connectionConfig_->dataCall->getInterfaceName())) {
      return false;
    }
    // Set socket options BEFORE bind
    setsockopt(this->serverSocket_, SOL_SOCKET, SO_REUSEADDR, &reuse,
               sizeof(reuse));
    setsockopt(this->serverSocket_, SOL_SOCKET, SO_REUSEPORT, &reuse,
               sizeof(reuse));

    if (this->connectionConfig_->ipFamily == telux::data::IpFamilyType::IPV4) {
      struct sockaddr_in v4ServerAddr = {};
      if (!this->connectionConfig_->dataCall->getIpv4Info().addr.ifAddress.empty()) {
        inet_pton(AF_INET, this->connectionConfig_->dataCall->getIpv4Info().addr.ifAddress.c_str(),
                  &(v4ServerAddr.sin_addr));
      }
      v4ServerAddr.sin_family = AF_INET;
      v4ServerAddr.sin_port = htons(this->connectionConfig_->serverPort);
      sockAddr = reinterpret_cast<struct sockaddr *>(&v4ServerAddr);
      sockSize = sizeof(sockaddr_in);
    } else {
      struct sockaddr_in6 v6ServerAddr = {};
      if (!this->connectionConfig_->dataCall->getIpv6Info().addr.ifAddress.empty()) {
        inet_pton(AF_INET6, this->connectionConfig_->dataCall->getIpv6Info().addr.ifAddress.c_str(),
                  &(v6ServerAddr.sin6_addr));
      }
      v6ServerAddr.sin6_family = AF_INET6;
      v6ServerAddr.sin6_port = htons(this->connectionConfig_->serverPort);
      sockAddr = reinterpret_cast<struct sockaddr *>(&v6ServerAddr);
      sockSize = sizeof(sockaddr_in6);
    }

    if (bind(this->serverSocket_, sockAddr, sockSize) < 0) {
      LOG(ERROR, __FUNCTION__, " bind : ", std::string(strerror(errno)));
      this->isConnected_ = false;
      return false;
    }

    return true;
  }

  void listenTCPSync() {
    LOG(DEBUG, __FUNCTION__);

    if (listen(this->serverSocket_, 3) < 0) {
      LOG(ERROR, __FUNCTION__,
          " listen failed: ", std::string(strerror(errno)));
      return;
    }

    do {
      struct sockaddr_storage clientAddr;
      socklen_t clientAddrLen = sizeof(clientAddr);

      this->clientSocket_ = accept(
          this->serverSocket_, reinterpret_cast<struct sockaddr *>(&clientAddr),
          &clientAddrLen);
      if (this->clientSocket_ < 0) {
        LOG(ERROR, __FUNCTION__,
            " accept failed: ", std::string(strerror(errno)));
        continue;
      }

      // Call bindToDevice after accept
      if (!bindToDevice(this->clientSocket_, connectionConfig_->dataCall->getInterfaceName())) {
        LOG(ERROR, __FUNCTION__, " Failed to bind to device");
        close(this->clientSocket_);
        continue;
      }

      char clientIp[INET6_ADDRSTRLEN] = {};
      uint16_t clientPort = 0;

      if (clientAddr.ss_family == AF_INET) {
        struct sockaddr_in *addr =
            reinterpret_cast<struct sockaddr_in *>(&clientAddr);
        inet_ntop(AF_INET, &addr->sin_addr, clientIp, sizeof(clientIp));
        clientPort = ntohs(addr->sin_port);
      } else if (clientAddr.ss_family == AF_INET6) {
        struct sockaddr_in6 *addr =
            reinterpret_cast<struct sockaddr_in6 *>(&clientAddr);
        inet_ntop(AF_INET6, &addr->sin6_addr, clientIp, sizeof(clientIp));
        clientPort = ntohs(addr->sin6_port);
      }

      // Filter by IP and port if configured
      if (!this->connectionConfig_->clientIpAddr.empty() &&
          strcmp(clientIp, this->connectionConfig_->clientIpAddr.c_str()) != 0) {
        LOG(WARNING, __FUNCTION__, " rejected client IP: ", clientIp);
        close(this->clientSocket_);
        continue;
      }

      if (this->connectionConfig_->clientPort != 0 &&
          this->connectionConfig_->clientPort != clientPort) {
        LOG(WARNING, __FUNCTION__, " rejected client port: ", clientPort);
        close(this->clientSocket_);
        continue;
      }

      this->isConnected_ = true;
      updateConnectionParams();

      try {
        do {
          IPMessage msg;
          memset(&msg, 0, sizeof(msg));
          ssize_t n = recv(this->clientSocket_, static_cast<void *>(&msg),
                           sizeof(msg), 0);
          if (n <= 0) {
            LOG(ERROR, __FUNCTION__, " trigger connection interrupted ");
            this->isConnected_ = false;
            break;
          }
          for (auto listener : listeners_) {
            listener->messageReceived(msg, n, connectionConfig_);
          }
          LOG(DEBUG, __FUNCTION__, " length = ", n);
        } while (true);
      } catch (const std::exception &e) {
        this->isConnected_ = false;
        LOG(ERROR, __FUNCTION__, " exception: ", std::string(e.what()));
      }

      // Always clean up socket
      if (shutdown(this->clientSocket_, SHUT_RDWR) == -1) {
        LOG(ERROR, __FUNCTION__,
            " shutdown failed: ", std::string(strerror(errno)));
      }
      if (close(this->clientSocket_) == -1) {
        LOG(ERROR, __FUNCTION__,
            " close failed: ", std::string(strerror(errno)));
      }
      for (auto listener : listeners_) {
        listener->onDisconnect(this->connectionConfig_);
      }

    } while (!this->isReceivedStopServer_);

    close(this->serverSocket_);
    LOG(DEBUG, __FUNCTION__, " exit ");
  }

  bool updateConnectionParams() {
    LOG(DEBUG, __FUNCTION__);
    int soc = clientSocket_;

    char ipStr[INET6_ADDRSTRLEN];
    uint16_t port;
    struct sockaddr_storage localAddr;
    socklen_t addrLen = sizeof(localAddr);

    if (getsockname(soc, (struct sockaddr *)&localAddr, &addrLen) == -1) {
      LOG(ERROR, __FUNCTION__, " getsockname failed: ", strerror(errno));
      return false;
    }

    if (localAddr.ss_family == AF_INET) {
      struct sockaddr_in *addr_in = (struct sockaddr_in *)&localAddr;
      inet_ntop(AF_INET, &(addr_in->sin_addr), ipStr, sizeof(ipStr));
      port = ntohs(addr_in->sin_port);
      connectionConfig_->ipFamily = telux::data::IpFamilyType::IPV4;
    } else if (localAddr.ss_family == AF_INET6) {
      struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&localAddr;
      inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ipStr, sizeof(ipStr));
      port = ntohs(addr_in6->sin6_port);
      connectionConfig_->ipFamily = telux::data::IpFamilyType::IPV6;
    } else {
      LOG(ERROR, __FUNCTION__, " Unknown address family");
      return false;
    }

    connectionConfig_->serverIpAddr = std::string(ipStr);
    connectionConfig_->serverPort = port;

    // Get client info
    struct sockaddr_storage peerAddr;
    addrLen = sizeof(peerAddr);
    if (getpeername(soc, (struct sockaddr *)&peerAddr, &addrLen) == -1) {
      LOG(ERROR, __FUNCTION__, " getpeername failed: ", strerror(errno));
      return false;
    }

    if (peerAddr.ss_family == AF_INET) {
      struct sockaddr_in *addr_in = (struct sockaddr_in *)&peerAddr;
      inet_ntop(AF_INET, &(addr_in->sin_addr), ipStr, sizeof(ipStr));
      port = ntohs(addr_in->sin_port);
    } else if (peerAddr.ss_family == AF_INET6) {
      struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&peerAddr;
      inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ipStr, sizeof(ipStr));
      port = ntohs(addr_in6->sin6_port);
    } else {
      LOG(ERROR, __FUNCTION__, " Unknown peer address family");
      return false;
    }

    connectionConfig_->clientIpAddr = std::string(ipStr);
    connectionConfig_->clientPort = port;
    return true;
  }
};
#endif // TCPSERVER_HPP

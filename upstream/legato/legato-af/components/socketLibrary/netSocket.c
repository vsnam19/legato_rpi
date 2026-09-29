/**
 * @file netSocket.c
 *
 * This file implements a set of networking functions to manage unsecure TCP/UDP sockets.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "netSocket.h"

#if LE_CONFIG_LINUX
#include <netdb.h>
#include <netinet/tcp.h>
#endif
#include <arpa/inet.h>

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Port maximum length
 */
//--------------------------------------------------------------------------------------------------
#define PORT_STR_LEN      6
#define PENDING_CONNECTION_NUM  5

static le_result_t GetSrcAddrInfo
(
    const char*              srcAddrPtr,       ///< [IN] Source address pointer
    int*                     sockFamilyPtr,    ///< [OUT] Socket address family pointer
    struct sockaddr_storage* socketAddrPtr     ///< [OUT] Socket address info pointer
)
{
    le_result_t result = LE_FAULT;

    // Get socket address and family from source IP string
    if (inet_pton(AF_INET, srcAddrPtr, &(((struct sockaddr_in *)socketAddrPtr)->sin_addr)) == 1)
    {
        LE_INFO("GetSrcAddrInfo address is IPv4 %s", srcAddrPtr);
        ((struct sockaddr_in *)socketAddrPtr)->sin_family = AF_INET;
        *sockFamilyPtr = AF_INET;
        result = LE_OK;
    }
    else if (inet_pton(AF_INET6, srcAddrPtr, &(((struct sockaddr_in6 *)socketAddrPtr)->sin6_addr))
        == 1)
    {
        LE_INFO("GetSrcAddrInfo address is IPv6 %s", srcAddrPtr);
        ((struct sockaddr_in *)socketAddrPtr)->sin_family = AF_INET6;
        *sockFamilyPtr = AF_INET6;
        result = LE_OK;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
// Public functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Initiate a connection with host:port and the given protocol.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_UNAVAILABLE   Unable to reach the server or DNS issue
 *  - LE_FAULT         Internal error
 *  - LE_COMM_ERROR    Connection failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Connect
(
    const char*     hostPtr,        ///< [IN] Host address pointer
    uint16_t        port,           ///< [IN] Port number
    const char*     localAddrPtr,   ///< [IN] Local address pointer
    SocketType_t    type,           ///< [IN] Socket type (TCP, UDP)
    int*            fdPtr           ///< [OUT] Socket file descriptor
)
{
    char portStr[PORT_STR_LEN] = {0};
    struct addrinfo hints, *addrList, *curPtr;
    struct sockaddr_storage srcSockAddr = {0};
    int fd = -1;

    if ((hostPtr == NULL) || (fdPtr == NULL))
    {
        LE_ERROR("Wrong parameter provided: %p, %p", hostPtr, fdPtr);
        return LE_BAD_PARAMETER;
    }

    // Convert port to string
    snprintf(portStr, PORT_STR_LEN, "%hu", port);

    // Do name resolution with both IPv6 and IPv4
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = (type == UDP_TYPE) ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = (type == UDP_TYPE) ? IPPROTO_UDP : IPPROTO_TCP;

    // Initialize socket structure from source IP string
    if (GetSrcAddrInfo(localAddrPtr, &(hints.ai_family), &srcSockAddr) != LE_OK)
    {
        LE_ERROR("Error on function: GetSrcAddrInfo");
        return LE_FAULT;
    }

    // Resolve name
    if(getaddrinfo(hostPtr, portStr, &hints, &addrList) != 0)
    {
        LE_ERROR("Error on function: getaddrinfo");
        return LE_UNAVAILABLE;
    }

    // Try the sockaddrs until a connection succeeds
    for(curPtr = addrList; curPtr != NULL; curPtr = curPtr->ai_next)
    {
        // Create socket
        fd = socket(curPtr->ai_family, curPtr->ai_socktype, 0);
        if (fd < 0)
        {
            continue;
        }

        if (bind(fd, (struct sockaddr*)&srcSockAddr, sizeof(srcSockAddr)) != 0)
        {
            LE_ERROR("Bind socket error(%s)", strerror(errno));
            close(fd);
            fd = -1;
            continue;
        }

        if (connect(fd, curPtr->ai_addr, curPtr->ai_addrlen) != 0)
        {
            LE_ERROR("Connect socket error(%s)", strerror(errno));
            close(fd);
            fd = -1;
            continue;
        }
        break;
    }

    freeaddrinfo(addrList);

    if (fd == -1)
    {
        LE_ERROR("Failed to connect by Using all source address!");
        return LE_COMM_ERROR;
    }
    else
    {
        // Successful, output the fd and return ok.
        *fdPtr = fd;
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Gracefully close the socket connection
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Disconnect
(
    int fd    ///< [IN] Socket file descriptor
)
{
    if (fd == -1)
    {
        return LE_FAULT;
    }

    close(fd);
    return LE_OK;
}

/**
 * Gracefully shutdown the socket connection
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Shutdown
(
    int fd,      ///< [IN] Socket file descriptor
    int how      ///< [IN] Shutdown direction
)
{
    if (fd == -1)
    {
        return LE_FAULT;
    }

    if( (how != SHUT_RD) && (how != SHUT_WR) && (how != SHUT_RDWR))
    {
        return LE_BAD_PARAMETER;
    }

    shutdown(fd, how);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write to the socket file descriptor an amount of data in a blocking way
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Write
(
    int     fd,           ///< [IN] Socket file descriptor
    const char*   bufPtr, ///< [IN] Data pointer to be sent
    size_t  bufLen        ///< [IN] Size of data to be sent
)
{
    ssize_t count;

    if ((bufPtr == NULL) || (fd < 0))
    {
        return LE_BAD_PARAMETER;
    }

    while (bufLen)
    {
        count = write(fd, bufPtr, bufLen);
        if (count >= 0)
        {
            bufLen -= count;
            bufPtr += count;
        }
        else if (-1 == count && (EINTR == errno))
        {
            continue;
        }
        else
        {
            LE_ERROR("Write failed: %d, %s", errno, LE_ERRNO_TXT(errno));
            return LE_FAULT;
        }
    }

    LE_DEBUG("Write done successfully on fd: %d", fd);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read data from the socket file descriptor in a blocking way. If the timeout is zero, then the
 * API returns immediatly.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 *  - LE_TIMEOUT       Timeout during execution
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Read
(
    int     fd,           ///< [IN] Socket file descriptor
    char*   bufPtr,       ///< [OUT] Read buffer pointer
    size_t*  bufLenPtr,   ///< [INOUT] Input: size of the buffer. Output: data size read
    uint32_t timeout      ///< [IN] Read timeout in milliseconds.
)
{
    fd_set set;
    int rv, count;
    struct timeval time = {.tv_sec = timeout / 1000, .tv_usec = (timeout % 1000) * 1000};

    if ((bufPtr == NULL) || (bufLenPtr == NULL) || (fd < 0))
    {
        return LE_BAD_PARAMETER;
    }

    do
    {
       FD_ZERO(&set);
       FD_SET(fd, &set);
       rv = select(fd + 1, &set, NULL, NULL, &time);
    }
    while (rv == -1 && errno == EINTR);

    if (rv > 0)
    {
        if (FD_ISSET(fd, &set))
        {
            do
            {
               count = recv(fd, bufPtr, *bufLenPtr, 0);
            }
            while (count == -1 && errno == EINTR);

            if (count < 0)
            {
                LE_ERROR("Read failed: %d, %s", errno, LE_ERRNO_TXT(errno));
                return LE_FAULT;
            }

            *bufLenPtr = count;
        }
    }
    else if (rv == 0)
    {
        return LE_TIMEOUT;
    }
    else
    {
        return LE_FAULT;
    }

    LE_DEBUG("Read size: %"PRIuS, *bufLenPtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Join a multicast address to the socket file descriptor.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_UNAVAILABLE   Unable to reach the server or DNS issue
 *  - LE_FAULT         Internal error
 *  - LE_COMM_ERROR    Socket failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_JoinMulticast
(
    int         fd,         ///< [IN] Socket file descriptor
    const char* mcAddrPtr,  ///< [IN] Multicast address pointer
    const char* srcAddrPtr  ///< [IN] Locat address pointer
)
{
    int retcode;
    struct addrinfo hints;
    struct sockaddr_storage srcSocket = {0};

    if ((mcAddrPtr == NULL) || (srcAddrPtr == NULL))
    {
        LE_ERROR("Wrong parameter provided: %p, %p", mcAddrPtr, srcAddrPtr);
        return LE_BAD_PARAMETER;
    }

    // Do name resolution with both IPv6 and IPv4
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    // Initialize socket structure from source IP string
    if (GetSrcAddrInfo(srcAddrPtr, &(hints.ai_family), &srcSocket) != LE_OK)
    {
        LE_ERROR("Error on function: GetSrcAddrInfo");
        return LE_UNAVAILABLE;
    }


    if (hints.ai_family == AF_INET)
    {
        struct ip_mreq      mreq = {0};

        retcode = inet_pton(AF_INET, mcAddrPtr, &mreq.imr_multiaddr);
        if (retcode != 1)
        {
            LE_ERROR("Failed to translate multicast address - %d", retcode);
            return LE_FAULT;
        }
        mreq.imr_interface = ((struct sockaddr_in*)&srcSocket)->sin_addr;

        if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 0)
        {
            LE_ERROR("Failed to set socket membership option--%s", strerror(errno));
            return LE_FAULT;
        }
    }
    else if (hints.ai_family == AF_INET6)
    {
        struct ipv6_mreq    mreq = {0};

        retcode = inet_pton(AF_INET6, mcAddrPtr, &mreq.ipv6mr_multiaddr);
        if (retcode != 1)
        {
            LE_ERROR("Failed to translate multicast address - %d", retcode);
            return LE_FAULT;
        }
        mreq.ipv6mr_interface = 0;

        if (setsockopt(fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 0)
        {
            LE_ERROR("Failed to set membership socket option--%s", strerror(errno));
            return LE_FAULT;
        }
    }
    else
    {
        LE_ERROR("Unknow socket family - %d", hints.ai_family);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send to specific ip:port an amount of data via the socket file descriptor in a blocking way
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Sendto
(
    int         fd,          ///< [IN] Socket file descriptor
    const char* dataPtr,     ///< [IN] Data pointer
    size_t      dataLen,     ///< [IN] Size of data
    const char* ipAddrPtr,   ///< [IN] Destination ip to be sent
    uint16_t    port         ///< [IN] Destination port to be sent
)
{
    ssize_t cnt;
    struct sockaddr_storage destSocket;

    if ((dataPtr == NULL) || (fd < 0) || (ipAddrPtr == NULL))
    {
        return LE_BAD_PARAMETER;
    }

    // Translate the IP address to socket address.
    if (inet_pton(AF_INET, ipAddrPtr,
                  &(((struct sockaddr_in*)&destSocket)->sin_addr)) == 1)
    {
        ((struct sockaddr_in*)&destSocket)->sin_family  = AF_INET;
        ((struct sockaddr_in*)&destSocket)->sin_port    = htons(port);
    }
    else if (inet_pton(AF_INET6, ipAddrPtr,
                       &(((struct sockaddr_in6*)&destSocket)->sin6_addr)) == 1)
    {
        ((struct sockaddr_in6*)&destSocket)->sin6_family  = AF_INET6;
        ((struct sockaddr_in6*)&destSocket)->sin6_port    = htons(port);
    }
    else
    {
        LE_ERROR("Cannot convert address %s", ipAddrPtr);
        return LE_FAULT;
    }

    while (dataLen)
    {
        cnt = sendto(fd, dataPtr, dataLen, 0, (struct sockaddr*)&destSocket, sizeof(destSocket));
        if (cnt >= 0)
        {
            dataLen -= cnt;
            dataPtr += cnt;
        }
        else if (-1 == cnt && (EINTR == errno))
        {
            continue;
        }
        else
        {
            LE_ERROR("Sendto error: %d, %s", errno, LE_ERRNO_TXT(errno));
            return LE_FAULT;
        }
    }

    LE_DEBUG("Sendto done successfully on fd: %d", fd);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Receive data from the socket file descriptor and record peer address in a blocking way.
 *
 * @note If the timeout is zero, this API returns immediatly.
 *
 * @return
 *  - LE_OK            The function succeeded
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 *  - LE_TIMEOUT       Timeout during execution
 *  - LE_OVERFLOW      Ip address buffer is overflow
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Recvfrom
(
    int         fd,             ///< [IN] Socket file descriptor
    char*       dataPtr,        ///< [OUT] Data buffer pointer
    size_t*     dataLenPtr,     ///< [INOUT] Input: size of the data.
                                ///<         Output: data size read
    uint32_t    timeout,        ///< [IN] Read timeout in milliseconds.
    char*       ipAddrBufPtr,   ///< [OUT] Peer ip address
    size_t      ipAddrBufLen,   ///< [IN] Size of peer ip address buffer
    uint16_t*   portPtr         ///< [OUT] Peer port
)
{
    fd_set set;
    int rv, cnt;
    struct sockaddr_storage addr = {0};
    socklen_t addrLen = sizeof(addr);
    struct timeval tv = {.tv_sec = timeout / 1000, .tv_usec = (timeout % 1000) * 1000};

    if ((dataPtr == NULL) || (dataLenPtr == NULL) || (fd < 0)
        || (ipAddrBufPtr == NULL) || (portPtr == NULL))
    {
        return LE_BAD_PARAMETER;
    }

    do
    {
       FD_ZERO(&set);
       FD_SET(fd, &set);
       rv = select(fd + 1, &set, NULL, NULL, &tv);
    }
    while (rv == -1 && errno == EINTR);

    if (rv > 0)
    {
        if (FD_ISSET(fd, &set))
        {
            do
            {
               cnt = recvfrom(fd, dataPtr, *dataLenPtr, 0, (struct sockaddr*)&addr, &addrLen);
            }
            while (cnt == -1 && errno == EINTR);

            if (cnt < 0)
            {
                LE_ERROR("Recvfrom error: %d, %s", errno, LE_ERRNO_TXT(errno));
                return LE_FAULT;
            }

            *dataLenPtr = cnt;
        }
    }
    else if (rv == 0)
    {
        return LE_TIMEOUT;
    }
    else
    {
        return LE_FAULT;
    }

    if (((struct sockaddr_in *)&addr)->sin_family == AF_INET)
    {
        if (ipAddrBufLen < INET_ADDRSTRLEN)
        {
            LE_ERROR("Ip address buffer(%"PRIuS") is not enough.", ipAddrBufLen);
            return LE_OVERFLOW;
        }
        inet_ntop(AF_INET, &(((struct sockaddr_in*)&addr)->sin_addr),
                  ipAddrBufPtr, ipAddrBufLen);
        *portPtr = ntohs(((struct sockaddr_in*)&addr)->sin_port);
    }
    else if (((struct sockaddr_in *)&addr)->sin_family == AF_INET6)
    {
        if (ipAddrBufLen < INET6_ADDRSTRLEN)
        {
            LE_ERROR("Ip address buffer(%"PRIuS") is not enough.", ipAddrBufLen);
            return LE_OVERFLOW;
        }
        inet_ntop(AF_INET6, &(((struct sockaddr_in6*)&addr)->sin6_addr),
                       ipAddrBufPtr, ipAddrBufLen);
        *portPtr = ntohs(((struct sockaddr_in6*)&addr)->sin6_port);
    }
    else
    {
        LE_ERROR("netSocket_recvfrom cannot get address. Unknow address family-%d",
            ((struct sockaddr_in *)&addr)->sin_family);
        return LE_FAULT;
    }

    LE_DEBUG("Recvfrom size: %"PRIuS, *dataLenPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Creates a socket and bind to specified address and port.
 *
 * @note It will do listen operation if socket type is TCP.
 *
 * @return
 *  - LE_OK             Function success
 *  - LE_BAD_PARAMETER  Invalid parameter
 *  - LE_UNAVAILABLE    DNS issue.
 *  - LE_COMM_ERROR     Socket failure.
 *  - LE_NOT_PERMITTED  Operation is not permitted.
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Bind
(
    const char*     localAddrPtr,   ///< [IN] Local address pointer
    uint16_t        localPort,      ///< [IN] Local Port number
    SocketType_t    sockType,       ///< [IN] Socket type (TCP, UDP)
    int*            fdPtr           ///< [OUT] Socket file descriptor
)
{
    struct addrinfo svrAddrInfo = {0};  // Server address info.
    struct sockaddr_storage srcSockAddr = {0};
    int fd = -1;
    int opt;

    if ((localAddrPtr == NULL) || (fdPtr == NULL))
    {
        LE_ERROR("Wrong parameter provided: %p, %p", localAddrPtr, fdPtr);
        return LE_BAD_PARAMETER;
    }

    // Do name resolution with both IPv6 and IPv4
    svrAddrInfo.ai_family = AF_UNSPEC;

    if (sockType == UDP_TYPE)
    {
        svrAddrInfo.ai_socktype = SOCK_DGRAM;
        svrAddrInfo.ai_protocol = IPPROTO_UDP;
    }
    else if (sockType == TCP_TYPE)
    {
        svrAddrInfo.ai_socktype = SOCK_STREAM;
        svrAddrInfo.ai_protocol = IPPROTO_TCP;
    }
    else
    {
        LE_ERROR("Wrong parameter provided: %d", sockType);
        return LE_BAD_PARAMETER;
    }

    // Initialize socket structure from source IP string
    if (GetSrcAddrInfo(localAddrPtr, &(svrAddrInfo.ai_family), &srcSockAddr) != LE_OK)
    {
        LE_ERROR("Error on function: GetSrcAddrInfo");
        return LE_UNAVAILABLE;
    }

    if(AF_INET == svrAddrInfo.ai_family)
    {
        ((struct sockaddr_in*)&srcSockAddr)->sin_port = htons(localPort);
    }
    else
    {
        ((struct sockaddr_in6*)&srcSockAddr)->sin6_port = htons(localPort);
    }

    fd = socket(svrAddrInfo.ai_family, svrAddrInfo.ai_socktype, 0);
    if (fd < 0)
    {
        LE_ERROR("Unable to create a socket");
        return LE_COMM_ERROR;
    }

    opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0)
    {
        LE_ERROR("Failed to set reuse option--%s", strerror(errno));
        close(fd);
        return LE_COMM_ERROR;
    }

    // Bind socket to source address
    if(bind(fd, (struct sockaddr*)&srcSockAddr, sizeof(struct sockaddr_storage)) < 0)
    {
        LE_ERROR("ERROR binding to the socket (%d)", errno);
        close(fd);
        return LE_COMM_ERROR;
    }

    if (sockType == TCP_TYPE)
    {
        if (listen(fd, PENDING_CONNECTION_NUM) < 0)
        {
            LE_ERROR("ERROR listening to the socket (%d)", errno);
            close(fd);
            return LE_COMM_ERROR;
        }
    }
    else
    {
        opt = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) != 0)
        {
            LE_ERROR("Failed to set membership socket option--%s", strerror(errno));
            close(fd);
            return LE_COMM_ERROR;
        }
    }

    *fdPtr = fd;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Accept a connection from remote client and return the description file.
 *
 * @return
 *  - LE_OK            Function success.
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_UNAVAILABLE   Unable to accept a peer client socket.
 *  - LE_FAULT         Set socket option error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Accept
(
    int                 serverFd,       ///< [IN] Local server file description
    struct sockaddr*    clientAddrPtr,  ///< [OUT] Client address pointer
    socklen_t*          clientAddrLen,  ///< [INOUT] Socket address length
    int*                clientFdPtr     ///< [OUT] Accepted client socket file descriptor pointer
)
{
    int nodelay = 1;
    int clientFd = -1;

    if ((clientAddrPtr == NULL) || (clientAddrLen == NULL) || (clientFdPtr == NULL))
    {
        LE_ERROR("Wrong parameter provided");
        return LE_BAD_PARAMETER;
    }

    clientFd = accept(serverFd, clientAddrPtr, clientAddrLen);
    if(-1 == clientFd)
    {
        LE_ERROR("ERROR accepting the socket (%d)", errno);
        return LE_UNAVAILABLE;
    }

    if(0 != setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, (char *)&nodelay, sizeof(nodelay)))
    {
        close(clientFd);
        LE_ERROR("ERROR setting the socket option(%d)", errno);
        return LE_FAULT;
    }

    *clientFdPtr = clientFd;

    return LE_OK;
}

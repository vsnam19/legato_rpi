/**
 * @file netSocket.h
 *
 * Networking functions definition for unsecure TCP/UDP sockets.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef LE_NET_SOCKET_LIB_H
#define LE_NET_SOCKET_LIB_H

#include "legato.h"
#include "interfaces.h"
#include "common.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Public functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Initiate a connection with host:port and the given protocol
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_UNAVAILABLE   Unable to reach the server or DNS issue
 *  - LE_FAULT         Internal error
 *  - LE_COMM_ERROR    If connection failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t netSocket_Connect
(
    const char*     hostPtr,        ///< [IN] Host address pointer
    uint16_t        port,           ///< [IN] Port number
    const char*     localAddrPtr,   ///< [IN] Local address pointer
    SocketType_t    type,           ///< [IN] Socket type (TCP, UDP)
    int*            fdPtr           ///< [OUT] Socket file descriptor
);

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
);

//--------------------------------------------------------------------------------------------------
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
    int fd,       ///< [IN] Socket file descriptor
    int how       ///< [IN] Shutdown direction
);

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
    int     fd,         ///< [IN] Socket file descriptor
    const char* bufPtr, ///< [IN] Data pointer to be sent
    size_t  bufLen      ///< [IN] Size of data to be sent
);

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
);

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
);

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
);

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
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * Accept a connection from peer client and output the description file.
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
);

#endif /* LE_NET_SOCKET_LIB_H */

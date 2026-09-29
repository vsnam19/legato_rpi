/**
 * @page c_le_socket Socket library
 *
 * @ref le_socketLib.h "API Reference"
 *
 * <HR>
 *
 * @section socket_overview Overview
 *
 * Socket library provides a set of APIs to create sockets and transfer data with or without SSL
 * encryptions. Socket library features are:
 *
 * - Multi-app safe APIs
 * - SSL encryption
 * - Timeout management
 * - Socket monitoring
 *
 * @section socket_include Socket library include
 * @code
 *  requires:
 *  {
 *      component:
 *      {
 *          $LEGATO_ROOT/components/socketLibrary
 *      }
 *  }
 *
 *  cflags:
 *  {
 *      -I$LEGATO_ROOT/components/socketLibrary
 *  }
 * @endcode
 *
 * @section socket_reference Socket reference
 *
 * Since socket library is multi-app safe, a reference needs to be created in order to use the APIs.
 * First, call @ref le_socket_Create and specify the host address, the port and the socket type.
 * The returned context reference must be used later to configure the socket and send/receive data.
 * Call @ref le_socket_Delete to destroy the previously allocated context if not needed anymore.
 *
 * Example code:
 * @snippet "apps/test/httpServices/socketIntegrationTest/socketTestComponent/socketTest.c"
 * SocketCreate
 *
 * @section socket_certificate Socket certificate
 *
 * In order to enable SSL encryption on top of the socket, a valid DER encoded certificate must be
 * passed through @ref le_socket_AddCertificate. This API decodes the certificate and enables
 * secure exchanges. It is possible to pass several DER certificates for the same socket reference.
 *
 * Example code:
 * @snippet "apps/test/httpServices/socketIntegrationTest/socketTestComponent/socketTest.c"
 * SocketCertificate
 *
 * @section socket_connect Socket connect
 *
 * Once a reference is created and optionally a socket certificate is injected, user application
 * can connect the socket to the remote server by calling @ref le_socket_Connect and disconnect
 * it later by calling @ref le_socket_Disconnect.
 *
 * Example code:
 * @snippet "apps/test/httpServices/socketIntegrationTest/socketTestComponent/socketTest.c"
 * SocketConnect
 *
 * Data transmission can be achieved through @ref le_socket_Read and @ref le_socket_Send APIs.
 * These APIs are blocking until there is something to read from the socket or send is finished.
 * A default timeout of 10 sec is implemented to prevent infinite wait. This duration can be
 * modified by calling @ref le_httpClient_SetTimeout API.
 *
 * @section socket_monitoring Socket monitoring
 *
 * Although it's common to block a thread on a call to @ref le_socket_Read, it also blocks other
 * components running on the same thread. To avoid this situation, it's possible to create a
 * dedicated thread for blocking calls or use socket monitoring which is specifically designed
 * for this purpose.
 * Socket monitor component monitors socket file descriptors and reports to subscribed applications
 * when a new event fires. The list of handled events are:
 *
 * - @c POLLIN    = Data available to read in the socket
 * - @c POLLOUT   = Possible to send data on the socket
 * - @c POLLPRI   = Out of band data received only on TCP
 * - @c POLLRDHUP = Peer closed the connection in a connection-orientated socket.
 *
 * These are bitmask values and can be combined using the bit-wise OR operator ('|') and tested
 * for using the bit-wise @e and ('&') operator.
 *
 * To use socket monitoring, user application must subscribe a handler in order to received the
 * listed events through @ref le_socket_AddEventHandler. Then, @ref le_socket_SetMonitoring API
 * must be called to enable monitoring.
 *
 * Example code:
 * @snippet "apps/test/httpServices/socketIntegrationTest/socketTestComponent/socketTest.c"
 * SocketMonitoring
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

//--------------------------------------------------------------------------------------------------
/**
 * @file le_socketLib.h
 *
 * Socket library interface. APIs definition.
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_SOCKET_LIB_H
#define LE_SOCKET_LIB_H

#include "legato.h"
#include "interfaces.h"
#include "common.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of sockets. Note that increasing this value increases memory consumption
 */
//--------------------------------------------------------------------------------------------------
#define MAX_SOCKET_NB            LE_CONFIG_SOCKET_LIB_SESSION_MAX

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of host address
 */
//--------------------------------------------------------------------------------------------------
#define HOST_ADDR_LEN            255

//--------------------------------------------------------------------------------------------------
/**
 * Default communication timeout in milliseconds
 */
//--------------------------------------------------------------------------------------------------
#define COMM_TIMEOUT_DEFAULT_MS         10000

//--------------------------------------------------------------------------------------------------
/**
 * Reference type for sockets
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_socket_Ref* le_socket_Ref_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Event handler definition to monitor input and output data availability for sockets.
 *  Managed events are the same as in @ref le_fdMonitor.h. These events are:
 *
 * - @c POLLIN    = Data available to read in the socket
 * - @c POLLOUT   = Possible to send data on the socket
 * - @c POLLPRI   = Out of band data received only on TCP
 * - @c POLLRDHUP = Peer closed the connection in a connection-orientated socket.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_socket_EventHandler_t)
(
    le_socket_Ref_t  ref,       ///< [IN] Socket context reference
    short            events,    ///< [IN] Bitmap of events that occurred
    void*            userPtr    ///< [IN] User-defined pointer
);

//--------------------------------------------------------------------------------------------------
// Public functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Create a a socket reference and stores the user configuration in a dedicated context.
 *
 * @note
 *    hostPtr must be set to specify the server IP or name for a client socket.
 *    It can be NULL for a server socket.
 *
 * @return
 *  - Reference to the created context
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_socket_Ref_t le_socket_Create
(
    char*           hostPtr,         ///< [IN] Host address pointer
    uint16_t        port,            ///< [IN] Port number
    char*           srcAddr,         ///< [IN] Source IP address
    SocketType_t    type             ///< [IN] Socket type (TCP, UDP)
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete a previously created socket and free allocated resources.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_Delete
(
    le_socket_Ref_t    ref   ///< [IN] Socket context reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Add a certificate to the socket in order to make the connection secure
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FORMAT_ERROR  Invalid certificate
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_AddCertificate
(
    le_socket_Ref_t   ref,             ///< [IN] Socket context reference
    const uint8_t*    certificatePtr,  ///< [IN] Certificate Pointer
    size_t            certificateLen   ///< [IN] Certificate Length
);

//--------------------------------------------------------------------------------------------------
/**
 * Initiate a connection with the server using the defined configuration.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_TIMEOUT       Timeout during execution
 *  - LE_UNAVAILABLE   Unable to reach the server or DNS issue
 *  - LE_FAULT         Internal error
 *  - LE_NO_MEMORY     Memory allocation issue
 *  - LE_CLOSED        In case of end of file error
 *  - LE_COMM_ERROR    Connection failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_Connect
(
    le_socket_Ref_t    ref   ///< [IN] Socket context reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Shutdown the socket connection.
 *
 * - @c SHUT_RD    = Disables further receive operations
 * - @c SHUT_WR    = Disables further send operations
 * - @c SHUT_RDWR  = Disables further send and receive operations
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_UNSUPPORTED   Not supported
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_Shutdown
(
    le_socket_Ref_t    ref,   ///< [IN] Socket context reference
    int                how    ///< [IN] Shutdown direction
);

//--------------------------------------------------------------------------------------------------
/**
 * Close the socket connection.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_Disconnect
(
    le_socket_Ref_t    ref   ///< [IN] Socket context reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Send data through the socket
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_TIMEOUT       Timeout during execution
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_Send
(
    le_socket_Ref_t  ref,        ///< [IN] Socket context reference
    const char*      dataPtr,    ///< [IN] Data pointer
    size_t           dataLen     ///< [IN] Data length
);

//--------------------------------------------------------------------------------------------------
/**
 * Read up to 'dataLenPtr' characters from the socket
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_TIMEOUT       Timeout during execution
 *  - LE_FAULT         Internal error
 *  - LE_WOULD_BLOCK   Would have blocked if non-blocking behaviour was not requested
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_Read
(
    le_socket_Ref_t  ref,        ///< [IN] Socket context reference
    char*            dataPtr,    ///< [OUT] Read buffer pointer
    size_t*          dataLenPtr  ///< [INOUT] Input: size of the buffer. Output: data size read
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the socket communication timeout. This timeout specifies the interval that the read API
 * should block waiting for data reception.
 *
 * @note If this interval is set to zero, then the read API returns immediatly.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_SetTimeout
(
    le_socket_Ref_t  ref,       ///< [IN] Socket context reference
    uint32_t         timeout    ///< [IN] Timeout in milliseconds
);

//--------------------------------------------------------------------------------------------------
/**
 * Enable or disable monitoring on the socket file descriptor. By default, monitoring is disabled.
 *
 * @note When monitoring is activated, socket events (e.g: POLLIN, POLLOUT, POLLRDHUP, ..)
 *       can be retrieved by using le_socket_AddEventHandler() API.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_DUPLICATE     Request already executed
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_SetMonitoring
(
    le_socket_Ref_t  socketRef,      ///< [IN] Socket context reference
    bool             enable          ///< [IN] True to activate monitoring, false otherwise
);

//--------------------------------------------------------------------------------------------------
/**
 * Check whether the socket monitoring is enabled or not
 *
 * @return
 *  - True if monitoring is enabled, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED bool le_socket_IsMonitoring
(
    le_socket_Ref_t  socketRef      ///< [IN] Socket context reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Add a handler to monitor socket events.
 *
 * @note Monitoring is performed by event loop. Thus, any thread that calls this API should provide
 *       an event loop to catch socket events.
 *
 * @note Check @c le_socket_EventHandler_t prototype definition to get the list of monitored events.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_AddEventHandler
(
    le_socket_Ref_t          socketRef,       ///< [IN] Socket context reference
    le_socket_EventHandler_t handlerFunc,     ///< [IN] Handler function
    void*                    userPtr          ///< [IN] User-defined data pointer
);

//--------------------------------------------------------------------------------------------------
/**
 * Trigger a call to the monitoring event handler when POLLOUT is ready again
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_TrigMonitoring
(
    le_socket_Ref_t          socketRef       ///< [IN] Socket context reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Initiate a server reception by Binding to a specified address and port.
 *
 * @note It will do listen operation if socket type is TCP.
 *
 * @return
 *  - LE_OK               Function success
 *  - LE_BAD_PARAMETER    Invalid parameter
 *  - LE_FAULT            Internal error
 *  - LE_UNAVAILABLE      Unable to reach the server or DNS issue
 *  - LE_COMM_ERROR       Socket failure
 *  - LE_NOT_PERMITTED    Function not permitted
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_Bind
(
    le_socket_Ref_t    ref       ///< [IN] Socket context reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Receive a connection from remote client.
 * It will generate a child socket reference for reception and sending on the connection.
 *
 * @return
 *  - Reference to the client socket    Success
 *  - NULL                              Failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_socket_Ref_t le_socket_Accept
(
    le_socket_Ref_t   serverRef,        ///< [IN]  Server socket reference.
    char*             clientAddrBufPtr, ///< [OUT] Client's IP address buffer pointer.
    size_t            clientAddrBufLen, ///< [IN] Size of the client's IP address buffer.
    int*              clientPort        ///< [OUT] Client's port number.
);

//--------------------------------------------------------------------------------------------------
/**
 * Join a multicast address to the socket reference. So it can receive multicast packets.
 *
 * @note Multicast IP address shall be set in legal scope.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_UNAVAILABLE   Unable to reach the server or DNS issue
 *  - LE_FAULT         Internal error
 *  - LE_COMM_ERROR    Socket failure
 *  - LE_NOT_PERMITTED Function not permitted
 *  - LE_CLOSED           Socket resource closed or not created
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_JoinMulticastGroup
(
    le_socket_Ref_t     ref,         ///< [IN] Socket context reference
    const char*         mcAddrPtr    ///< [IN] Multicast IP address
);

//--------------------------------------------------------------------------------------------------
/**
 * Receive data from the socket and output peer address.
 *
 * @note Only supported for UDP type
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_NOT_PERMITTED Function not permitted
 *  - LE_TIMEOUT       Timeout during execution
 *  - LE_FAULT         Internal error
 *  - LE_WOULD_BLOCK   Would have blocked if non-blocking behaviour was not requested
 *  - LE_CLOSED        Socket resource closed or not created.
 *  - LE_OVERFLOW      Ip address buffer is overflow
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_RecvFrom
(
    le_socket_Ref_t  ref,           ///< [IN] Socket context reference
    char*            dataPtr,       ///< [OUT] Data buffer pointer
    size_t*          dataLenPtr,    ///< [INOUT] Input: size of the data buffer.
                                    ///<         Output: data size is read
    char*            ipAddrBufPtr,  ///< [OUT] Peer address pointer
    size_t           ipAddrBufLen,  ///< [IN] Size of peer address buffer.
    uint16_t*        portPtr        ///< [OUT] Peer port
);

//--------------------------------------------------------------------------------------------------
/**
 * Send data to the peer address.
 *
 * @note Only supported for UDP type
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_NOT_PERMITTED Function not permitted
 *  - LE_TIMEOUT       Timeout during execution
 *  - LE_FAULT         Internal error
 *  - LE_WOULD_BLOCK   Would have blocked if non-blocking behaviour was not requested
 *  - LE_CLOSED        Socket resource closed or not created.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_socket_SendTo
(
    le_socket_Ref_t  ref,           ///< [IN] Socket context reference
    const char*      dataPtr,       ///< [IN] Data buffer pointer
    size_t           dataLen,       ///< [IN] Data length
    const char*      ipAddrPtr,     ///< [IN] Peer address pointer
    uint16_t         port           ///< [IN] Peer port
);

#endif  // LE_SOCKET_LIB_H

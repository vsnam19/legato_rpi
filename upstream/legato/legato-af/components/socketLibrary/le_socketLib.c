/**
 * @file le_socketLib.c
 *
 * Socket library interface for secure/unsecure connections using TCP or UDP connection types.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "legato.h"
#include "interfaces.h"

#include "le_socketLib.h"
#include "netSocket.h"
#include "secSocket.h"

#include <arpa/inet.h>

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------
#define MAX_ADDR_LEN    46

//--------------------------------------------------------------------------------------------------
/**
 * Socket context
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_socket_Ref_t    reference;              ///< Safe reference to this object
    int                fd;                     ///< Socket file descriptor
    char               host[HOST_ADDR_LEN];    ///< Host address
    uint16_t           port;                   ///< Host port
    char               srcAddr[MAX_ADDR_LEN];  ///< Source IP address
    SocketType_t       type;                   ///< Socket type (TCP, UDP)
    uint32_t           timeout;                ///< Communication timeout in milliseconds
    bool               isSecure;               ///< True if the socket uses a certificate
    bool               isMonitoring;           ///< True if the socket is being monitored
    le_fdMonitor_Ref_t monitorRef;             ///< Reference to the monitor object
    secSocket_Ctx_t*   secureCtxPtr;           ///< Secure socket context pointer
    short              events;                 ///< Bitmap of events that occurred
    void*              userPtr;                ///< User-defined pointer for socket event handler
    le_socket_EventHandler_t eventHandler;     ///< User-defined callback for ocket event handler
}
SocketCtx_t;

//--------------------------------------------------------------------------------------------------
// Internal variables
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for sockets.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(SocketPool, MAX_SOCKET_NB, sizeof(SocketCtx_t));

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool reference for the sockets pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SocketPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for the sockets pool.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t SocketRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Retrigger socket event handler in case more data needs to be read from secure socket
 */
//--------------------------------------------------------------------------------------------------
static void ReadMoreAsyncData
(
    void*                   param1Ptr,  ///< [IN] Socket context pointer
    void*                   param2Ptr   ///< [IN] Unused parameter
);

//--------------------------------------------------------------------------------------------------
// Internal functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pick an unused socket context from the socket pool and return it.
 *
 * @return Socket file descriptor
 */
//--------------------------------------------------------------------------------------------------
static SocketCtx_t* NewSocketContext
(
    void
)
{
    SocketCtx_t* contextPtr = NULL;

    // Alloc memory from pool
    contextPtr = le_mem_ForceAlloc(SocketPoolRef);

    if (NULL == contextPtr)
    {
        LE_ERROR("Unable to allocate a socket context from pool");
        return NULL;
    }

    memset(contextPtr, 0x00, sizeof(SocketCtx_t));

    // Create a safe reference for this object
    contextPtr->reference = le_ref_CreateRef(SocketRefMap, contextPtr);

    return contextPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Free a socket context and make it available for future use.
 *
 * @return Socket file descriptor
 */
//--------------------------------------------------------------------------------------------------
static void FreeSocketContext
(
    SocketCtx_t*    contextPtr    ///< [IN] Socket context pointer
)
{
    le_ref_DeleteRef(SocketRefMap, contextPtr->reference);
    memset(contextPtr, 0x00, sizeof(SocketCtx_t));
    le_mem_Release(contextPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Find socket context given its file descriptor.
 *
 * @return Socket file descriptor
 */
//--------------------------------------------------------------------------------------------------
static SocketCtx_t* FindSocketContext
(
    int fd      ///< [IN] Socket file descriptor
)
{
    // Check input parameter
    if (fd == -1)
    {
        LE_WARN("Uninitialized socket file descriptor");
        return NULL;
    }

    // Initialize the socket pool and the socket reference map if not yet done
    if (!SocketPoolRef)
    {
        SocketPoolRef = le_mem_InitStaticPool(SocketPool, MAX_SOCKET_NB, sizeof(SocketCtx_t));
        SocketRefMap = le_ref_CreateMap("le_socketLibMap", MAX_SOCKET_NB);
    }

    SocketCtx_t* contextPtr = NULL;
    le_ref_IterRef_t iterator  = le_ref_GetIterator(SocketRefMap);

    // Find and return socket context that uses the input file descriptor
    while (le_ref_NextNode(iterator) == LE_OK)
    {
        contextPtr = (SocketCtx_t *)le_ref_GetValue(iterator);
        if (!contextPtr)
        {
            continue;
        }

        if (contextPtr->fd == fd)
        {
            return contextPtr;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sockets events handler
 */
//--------------------------------------------------------------------------------------------------
static void SocketEventsHandler
(
    int fd,           ///< [IN] Socket file descriptor
    short events      ///< [IN] Bitmap of events that occurred
)
{
    SocketCtx_t* contextPtr = FindSocketContext(fd);
    if (!contextPtr)
    {
        return;
    }

    if (events & POLLOUT)
    {
        // In le_fdMonitor component, POLLOUT event is raised continuously when writing to the FD
        // is possible. The event is repeated indefinitely. Here, when POLLOUT event is received,
        // we disable it right after. This way, the notification is sent only once.
        if (contextPtr->monitorRef)
        {
            le_fdMonitor_Disable(contextPtr->monitorRef, POLLOUT);
        }
    }

    if (events & POLLRDHUP)
    {
        le_fdMonitor_Delete(contextPtr->monitorRef);
        contextPtr->monitorRef = NULL;
    }

    if (contextPtr->eventHandler)
    {
        contextPtr->eventHandler(contextPtr->reference, events, contextPtr->userPtr);

        // In secure context, low-level layers may read more data from socket than the data size
        // requested by client application. In this case, we need to notify again until all
        // the data is consumed
        if (contextPtr->isSecure)
        {
            if (secSocket_IsDataAvailable(contextPtr->secureCtxPtr))
            {
                contextPtr->events = events;
                le_event_QueueFunction(ReadMoreAsyncData, (void*)contextPtr->reference, NULL);

                // Disable POLLIN monitoring to prevent race condition between FD event and queued
                // function. POLLIN is renabled when the runloop calls ReadMoreAsyncData()
                if (contextPtr->monitorRef)
                {
                    le_fdMonitor_Disable(contextPtr->monitorRef, POLLIN);
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Re-trigger socket event handler in case more data needs to be read from secure socket
 */
//--------------------------------------------------------------------------------------------------
static void ReadMoreAsyncData
(
    void*   param1Ptr,      ///< [IN] Socket context pointer
    void*   param2Ptr       ///< [IN] Unused parameter
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, param1Ptr);
    if (!contextPtr)
    {
        LE_WARN("Referenece not found");
        return;
    }

    if (!contextPtr->monitorRef)
    {
        LE_INFO("Monitoring disabled");
        return;
    }

    le_fdMonitor_Enable(contextPtr->monitorRef, POLLIN);
    SocketEventsHandler(contextPtr->fd, contextPtr->events);
}

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
le_socket_Ref_t le_socket_Create
(
    char*           hostPtr,         ///< [IN] Host address pointer
    uint16_t        port,            ///< [IN] Port number
    char*           srcAddr,         ///< [IN] Source IP address
    SocketType_t    type             ///< [IN] Socket type (TCP, UDP)
)
{
    SocketCtx_t* contextPtr = NULL;

    if (srcAddr == NULL)
    {
        LE_ERROR("Source IP address is NULL");
        return NULL;
    }

    if (type != TCP_TYPE && type != UDP_TYPE)
    {
        LE_ERROR("Unknow socket type%d", type);
        return NULL;
    }

    // Allocate a socket context and save server parameters
    contextPtr = NewSocketContext();
    if (NULL == contextPtr)
    {
        LE_ERROR("Unable to allocate a socket context from pool");
        return NULL;
    }

    if (strlen(srcAddr) >= sizeof(contextPtr->srcAddr))
    {
        LE_ERROR("Source address too long");
        FreeSocketContext(contextPtr);
        return NULL;
    }
    else
    {
        le_utf8_Copy(contextPtr->srcAddr, srcAddr, sizeof(contextPtr->srcAddr), NULL);
    }

    if (hostPtr)
    {
        le_utf8_Copy(contextPtr->host, hostPtr, sizeof(contextPtr->host)-1, NULL);
    }
    else
    {
        contextPtr->host[0] = '\0';
    }

    contextPtr->port      = port;
    contextPtr->type      = type;
    contextPtr->fd        = -1;
    contextPtr->timeout   = COMM_TIMEOUT_DEFAULT_MS;
    contextPtr->isMonitoring = false;

    return contextPtr->reference;
}

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
le_result_t le_socket_Delete
(
    le_socket_Ref_t    ref   ///< [IN] Socket context reference
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);

    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->monitorRef)
    {
        le_fdMonitor_Delete(contextPtr->monitorRef);
        contextPtr->monitorRef = NULL;
    }

    if (contextPtr->isSecure)
    {
        secSocket_Disconnect(contextPtr->secureCtxPtr);
    }
    else
    {
        netSocket_Disconnect(contextPtr->fd);
    }

    FreeSocketContext(contextPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add root CA certificates to the socket in order to make the connection secure.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_FORMAT_ERROR  Invalid certificate
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_AddCertificate
(
    le_socket_Ref_t   ref,             ///< [IN] Socket context reference
    const uint8_t*    certificatePtr,  ///< [IN] Certificate Pointer
    size_t            certificateLen   ///< [IN] Certificate Length
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);

    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if ((!certificatePtr) || (certificateLen == 0))
    {
        LE_ERROR("Wrong parameter: %p, %"PRIuS, certificatePtr, certificateLen);
        return LE_BAD_PARAMETER;
    }
    if (contextPtr->isSecure == 0)
    {
        // Need to initialize the secure socket before adding the certificate
        status = secSocket_Init(&(contextPtr->secureCtxPtr));
        if (status != LE_OK)
        {
            LE_ERROR("Unable to initialize the secure socket");
            return status;
        }

        contextPtr->isSecure = 1;
    }

    status = secSocket_AddCertificate(contextPtr->secureCtxPtr, certificatePtr, certificateLen);
    if (status != LE_OK)
    {
        LE_ERROR("Unable to add certificate");
    }

    return status;
}

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
le_result_t le_socket_Connect
(
    le_socket_Ref_t    ref   ///< [IN] Socket context reference
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);

    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (strlen(contextPtr->host) == 0)
    {
        LE_ERROR("Unknow server address.");
        return LE_UNAVAILABLE;
    }

    if (contextPtr->isSecure)
    {
        status = secSocket_Connect(contextPtr->secureCtxPtr, contextPtr->host,
                                   contextPtr->port, contextPtr->type, &(contextPtr->fd));
    }
    else
    {
        status = netSocket_Connect(contextPtr->host, contextPtr->port, contextPtr->srcAddr,
                                   contextPtr->type, &(contextPtr->fd));
    }

    if ((contextPtr->isMonitoring) && (!contextPtr->monitorRef))
    {
        contextPtr->monitorRef = le_fdMonitor_Create("SocketLibrary", contextPtr->fd,
                                                     SocketEventsHandler,
                                                     POLLIN | POLLRDHUP | POLLOUT);
        if (!contextPtr->monitorRef)
        {
            LE_ERROR("Unable to create an FD monitor object");
            return LE_FAULT;
        }
    }

    if (status != LE_OK)
    {
        LE_ERROR("Unable to connect");
    }

    return status;
}

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
le_result_t le_socket_Disconnect
(
    le_socket_Ref_t    ref   ///< [IN] Socket context reference
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->isSecure)
    {
        status = secSocket_Disconnect(contextPtr->secureCtxPtr);
    }
    else
    {
        status = netSocket_Disconnect(contextPtr->fd);
    }

    if (contextPtr->monitorRef)
    {
        le_fdMonitor_Delete(contextPtr->monitorRef);
        contextPtr->monitorRef = NULL;
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Shutdown the socket connection.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_UNSUPPORTED   Not supported
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_Shutdown
(
    le_socket_Ref_t    ref,   ///< [IN] Socket context reference
    int                how    ///< [IN] Shutdown direction
)
{
    le_result_t status = LE_OK;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if( (how != SHUT_RD) && (how != SHUT_WR) && (how != SHUT_RDWR))
    {
        LE_ERROR("Bad parameter how");
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->isSecure)
    {
        // TODO: Implement secure socket shutdown
        LE_WARN("Secure socket shutdown not implemented yet");
        status = LE_UNSUPPORTED;
    }
    else
    {
        status = netSocket_Shutdown(contextPtr->fd, how);
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send data through the socket.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_TIMEOUT       Timeout during execution
 *  - LE_FAULT         Internal error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_Send
(
    le_socket_Ref_t  ref,        ///< [IN] Socket context reference
    const char*      dataPtr,    ///< [IN] Data pointer
    size_t           dataLen     ///< [IN] Data length
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (!dataPtr)
    {
        LE_ERROR("Wrong parameter: %p", dataPtr);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->fd == -1)
    {
        LE_ERROR("Socket not connected");
        return LE_FAULT;
    }

    if (contextPtr->isMonitoring && contextPtr->monitorRef != NULL)
    {
        // Enable POLLOUT event just before sending data. Thus, when writing is possible again,
        // an event is raised.
        le_fdMonitor_Enable(contextPtr->monitorRef, POLLOUT);
    }

    if (contextPtr->isSecure)
    {
        status = secSocket_Write(contextPtr->secureCtxPtr, dataPtr, dataLen);
    }
    else
    {
        status = netSocket_Write(contextPtr->fd, dataPtr, dataLen);
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read up to 'dataLenPtr' characters from the socket in a blocking way until data is received or
 * defined timeout value is reached.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_BAD_PARAMETER Invalid parameter
 *  - LE_TIMEOUT       Timeout during execution
 *  - LE_FAULT         Internal error
 *  - LE_WOULD_BLOCK   Would have blocked if non-blocking behaviour was not requested
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_Read
(
    le_socket_Ref_t  ref,        ///< [IN] Socket context reference
    char*            dataPtr,    ///< [OUT] Read buffer pointer
    size_t*          dataLenPtr  ///< [INOUT] Input: size of the buffer. Output: data size read
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if ((!dataPtr) || (!dataLenPtr))
    {
        LE_ERROR("Wrong parameter: %p, %p", dataPtr, dataLenPtr);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->fd == -1)
    {
        LE_ERROR("Socket not connected");
        return LE_FAULT;
    }

    // Disable FD Monitor if it exists to avoid two different threads selecting the
    // same file descriptor
    if (contextPtr->monitorRef)
    {
        le_fdMonitor_Disable(contextPtr->monitorRef, POLLIN);
    }

    if (contextPtr->isSecure)
    {
        status = secSocket_Read(contextPtr->secureCtxPtr, dataPtr, dataLenPtr, contextPtr->timeout);
    }
    else
    {
        status = netSocket_Read(contextPtr->fd, dataPtr, dataLenPtr, contextPtr->timeout);
    }

    if ((status != LE_OK) && (status != LE_WOULD_BLOCK))
    {
        LE_ERROR("Read failed. Status: %d", status);
    }

    // Re-enable fdMonitor
    if (contextPtr->monitorRef)
    {
        le_fdMonitor_Enable(contextPtr->monitorRef, POLLIN);
    }

    return status;
}

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
le_result_t le_socket_SetTimeout
(
    le_socket_Ref_t  ref,       ///< [IN] Socket context reference
    uint32_t         timeout    ///< [IN] Timeout in milliseconds
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    contextPtr->timeout = timeout;
    return LE_OK;
}

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
le_result_t le_socket_SetMonitoring
(
    le_socket_Ref_t  socketRef,      ///< [IN] Socket context reference
    bool             enable          ///< [IN] True to activate monitoring, false otherwise
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, socketRef);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", socketRef);
        return LE_BAD_PARAMETER;
    }

    if ((contextPtr->monitorRef != NULL) == enable)
    {
        LE_INFO("Request was already executed");
        return LE_DUPLICATE;
    }

    if (enable)
    {
        // Check if the FD has already been created and connection started. In this case, FD
        // monitoring needs to be started immediatly. Otherwise, monitoring will be activated
        // after socket creation.
        if (contextPtr->fd != -1)
        {
            contextPtr->monitorRef = le_fdMonitor_Create("SocketLibrary", contextPtr->fd,
                                                         SocketEventsHandler,
                                                         POLLIN | POLLRDHUP | POLLOUT);
            if (!contextPtr->monitorRef)
            {
                LE_ERROR("Unable to create an FD monitor object");
                return LE_FAULT;
            }
        }
    }
    else
    {
        le_fdMonitor_Delete(contextPtr->monitorRef);
        contextPtr->monitorRef = NULL;
    }

    contextPtr->isMonitoring = enable;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check whether the socket monitoring is enabled or not
 *
 * @return
 *  - True if monitoring is enabled, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool le_socket_IsMonitoring
(
    le_socket_Ref_t  socketRef      ///< [IN] Socket context reference
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, socketRef);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", socketRef);
        return LE_BAD_PARAMETER;
    }

    return contextPtr->isMonitoring;
}

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
le_result_t le_socket_AddEventHandler
(
    le_socket_Ref_t          socketRef,       ///< [IN] Socket context reference
    le_socket_EventHandler_t handlerFunc,     ///< [IN] Handler function
    void*                    userPtr          ///< [IN] User-defined data pointer
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, socketRef);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", socketRef);
        return LE_BAD_PARAMETER;
    }

    contextPtr->userPtr = userPtr;
    contextPtr->eventHandler = handlerFunc;
    return LE_OK;
}

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
le_result_t le_socket_TrigMonitoring
(
    le_socket_Ref_t          socketRef       ///< [IN] Socket context reference
)
{
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, socketRef);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", socketRef);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->monitorRef == NULL)
    {
        LE_ERROR("Monitoring is not enabled");
        return LE_FAULT;
    }

    // Since POLLOUT event is sent continuously when writing to the FD is possible, enabling it
    // here ensures that SocketEventsHandler will be called right after.
    le_fdMonitor_Enable(contextPtr->monitorRef, POLLOUT);

    return LE_OK;
}


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
le_result_t le_socket_Bind
(
    le_socket_Ref_t    ref       ///< [IN] Socket context reference
)
{
    le_result_t status = LE_OK;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->fd == -1)
    {
        status = netSocket_Bind(contextPtr->srcAddr, contextPtr->port,
            contextPtr->type, &contextPtr->fd);
    }

    if ((contextPtr->isMonitoring) && (contextPtr->monitorRef == NULL))
    {
        contextPtr->monitorRef = le_fdMonitor_Create("SocketLibrary", contextPtr->fd,
                                                     SocketEventsHandler,
                                                     POLLIN | POLLRDHUP | POLLOUT);
        if (!contextPtr->monitorRef)
        {
            LE_ERROR("Unable to create an FD monitor object");
            return LE_FAULT;
        }
    }

    if (status != LE_OK)
    {
        LE_ERROR("Bind failed. Status: %d", status);
    }

    return status;
}

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
le_socket_Ref_t le_socket_Accept
(
    le_socket_Ref_t   serverRef,        ///< [IN]  Server socket reference.
    char*             clientAddrBufPtr, ///< [OUT] Client's IP address buffer pointer.
    size_t            clientAddrBufLen, ///< [IN] Size of the client's IP address buffer.
    int*              clientPort        ///< [OUT] Client's port number.
)
{
    le_result_t ret;
    SocketCtx_t *serverContextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, serverRef);
    SocketCtx_t* clientContextPtr = NULL;
    struct sockaddr_storage clientSockAddr;
    int clientFd = -1;
    char clientIpAddr[MAX_ADDR_LEN] = {0};
    socklen_t addrLen = sizeof(struct sockaddr_storage);

    if (serverContextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", serverRef);
        return NULL;
    }

    if (serverContextPtr->type != TCP_TYPE)
    {
        LE_ERROR("Only TCP socket supports accept.");
        return NULL;
    }

    if (serverContextPtr->fd == -1)
    {
        LE_ERROR("Socket fd is invalid.");
        return NULL;
    }

    ret = netSocket_Accept(serverContextPtr->fd,
                          (struct sockaddr*)&clientSockAddr,
                          &addrLen,
                          &clientFd);
    if(ret != LE_OK)
    {
        LE_ERROR("Failed to accept a client socket(%d)", ret);
        return NULL;
    }

    clientContextPtr = NewSocketContext();
    if (NULL == clientContextPtr)
    {
        LE_ERROR("Failed to allocate a socket context");
        close(clientFd);
        return NULL;
    }

    if (AF_INET == clientSockAddr.ss_family)
    {
        struct sockaddr_in *addrPtr = (struct sockaddr_in*)&clientSockAddr;

        if (clientAddrBufLen < INET_ADDRSTRLEN)
        {
            LE_ERROR("Ip address buffer(%"PRIuS") is not enough.", clientAddrBufLen);
            close(clientFd);
            return NULL;
        }

        inet_ntop(AF_INET, &(addrPtr->sin_addr), clientIpAddr, MAX_ADDR_LEN);
        clientContextPtr->port = (int)ntohs(addrPtr->sin_port);
    }
    else if (AF_INET6 == clientSockAddr.ss_family)
    {
        struct sockaddr_in6 *addrPtr = (struct sockaddr_in6*)&clientSockAddr;

        if (clientAddrBufLen < INET6_ADDRSTRLEN)
        {
            LE_ERROR("Ip address buffer(%"PRIuS") is not enough.", clientAddrBufLen);
            close(clientFd);
            return NULL;
        }

        inet_ntop(AF_INET6, &(addrPtr->sin6_addr), clientIpAddr, MAX_ADDR_LEN);
        clientContextPtr->port = (int)ntohs(addrPtr->sin6_port);
    }
    else
    {
        LE_ERROR("Unknown client socket family: %d.", clientSockAddr.ss_family);
        close(clientFd);
        return NULL;
    }

    clientContextPtr->type    = serverContextPtr->type;
    clientContextPtr->fd      = clientFd;
    clientContextPtr->timeout = COMM_TIMEOUT_DEFAULT_MS;
    clientContextPtr->isMonitoring = false;
    le_utf8_Copy(clientContextPtr->host, clientIpAddr, MAX_ADDR_LEN, NULL);
    le_utf8_Copy(clientAddrBufPtr, clientIpAddr, clientAddrBufLen, NULL);
    *clientPort = clientContextPtr->port;

    LE_INFO("Server has accepted a connection on fd:%d from [%s:%d",
        clientFd, clientContextPtr->host, clientContextPtr->port);

    return clientContextPtr->reference;
}

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
 *  - LE_CLOSED        Socket resource closed or not created.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_socket_JoinMulticastGroup
(
    le_socket_Ref_t     ref,         ///< [IN] Socket context reference
    const char*         mcAddrPtr    ///< [IN] Multicast IP address
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);

    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if (mcAddrPtr == NULL)
    {
        LE_ERROR("Multicast address not provided");
        return LE_BAD_PARAMETER;
    }

    if (strlen(contextPtr->srcAddr) == 0)
    {
        LE_ERROR("Failed to get source address");
        return LE_FAULT;
    }

    if (contextPtr->type != UDP_TYPE)
    {
        LE_ERROR("Only UDP socket supports multicast group.");
        return LE_NOT_PERMITTED;
    }

    if (contextPtr->fd == -1)
    {
        LE_ERROR("Socket fd is invalid.");
        return LE_CLOSED;
    }

    status = netSocket_JoinMulticast(contextPtr->fd, mcAddrPtr, contextPtr->srcAddr);
    if (status != LE_OK)
    {
        LE_ERROR("Unable to join multicast.");
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read up to 'dataLenPtr' characters from the socket and output destination address.
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
le_result_t le_socket_RecvFrom
(
    le_socket_Ref_t  ref,           ///< [IN] Socket context reference
    char*            dataPtr,       ///< [OUT] Read buffer pointer
    size_t*          dataLenPtr,    ///< [INOUT] Input: size of the buffer. Output: data size read
    char*            ipAddrBufPtr,  ///< [OUT] Peer address pointer
    size_t           ipAddrBufLen,  ///< [IN] Size of peer address buffer.
    uint16_t*        portPtr        ///< [OUT] Peer port
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if ((dataPtr == NULL) || (dataLenPtr == NULL))
    {
        LE_ERROR("Wrong parameter: %p, %p", dataPtr, dataLenPtr);
        return LE_BAD_PARAMETER;
    }

    if ((ipAddrBufPtr == NULL) || (portPtr == NULL))
    {
        LE_ERROR("Wrong parameter: %p, %p", ipAddrBufPtr, portPtr);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->type != UDP_TYPE)
    {
        LE_ERROR("Only UDP socket supports recvfrom.");
        return LE_NOT_PERMITTED;
    }

    if (contextPtr->fd == -1)
    {
        LE_ERROR("Socket fd is invalid.");
        return LE_CLOSED;
    }

    if (contextPtr->monitorRef != NULL)
    {
        le_fdMonitor_Disable(contextPtr->monitorRef, POLLIN);
    }

    status = netSocket_Recvfrom(contextPtr->fd, dataPtr, dataLenPtr,
        contextPtr->timeout, ipAddrBufPtr, ipAddrBufLen, portPtr);
    if ((status != LE_OK) && (status != LE_WOULD_BLOCK))
    {
        LE_ERROR("Failed to recvfrom. Status: %d", status);
    }

    // Re-enable fdMonitor
    if (contextPtr->monitorRef != NULL)
    {
        le_fdMonitor_Enable(contextPtr->monitorRef, POLLIN);
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send 'dataLen' characters to the peer address.
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
le_result_t le_socket_SendTo
(
    le_socket_Ref_t  ref,           ///< [IN] Socket context reference
    const char*      dataPtr,       ///< [IN] Read buffer pointer
    size_t           dataLen,       ///< [IN] Data length
    const char*      ipAddrPtr,     ///< [IN] Peer address pointer
    uint16_t         port           ///< [IN] Peer port
)
{
    le_result_t status;
    SocketCtx_t *contextPtr = (SocketCtx_t *)le_ref_Lookup(SocketRefMap, ref);
    if (contextPtr == NULL)
    {
        LE_ERROR("Reference not found: %p", ref);
        return LE_BAD_PARAMETER;
    }

    if ((dataPtr == NULL) || (ipAddrPtr == NULL))
    {
        LE_ERROR("Wrong parameter: %p, %p", dataPtr, ipAddrPtr);
        return LE_BAD_PARAMETER;
    }

    if (contextPtr->type != UDP_TYPE)
    {
        LE_ERROR("Only UDP socket supports multicast group.");
        return LE_NOT_PERMITTED;
    }

    if (contextPtr->fd == -1)
    {
        LE_ERROR("Socket fd is invalid.");
        return LE_CLOSED;
    }

    if ((contextPtr->isMonitoring) && (contextPtr->monitorRef != NULL))
    {
        le_fdMonitor_Enable(contextPtr->monitorRef, POLLOUT);
    }

    status = netSocket_Sendto(contextPtr->fd, dataPtr, dataLen, ipAddrPtr, port);

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initialization function
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_DEBUG("socketLibrary initializing");

    // Initialization socket resource.
    if (SocketPoolRef == NULL)
    {
        SocketPoolRef = le_mem_InitStaticPool(SocketPool, MAX_SOCKET_NB, sizeof(SocketCtx_t));
        SocketRefMap = le_ref_CreateMap("le_socketLibMap", MAX_SOCKET_NB);
    }
}

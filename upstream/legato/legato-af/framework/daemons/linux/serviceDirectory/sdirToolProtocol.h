//--------------------------------------------------------------------------------------------------
/** @file sdirToolProtocol.h
 *
 * Definitions related to the protocol used between the "sdir" tool and the Service Directory.
 *
 * <hr/>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef SDIR_TOOL_PROTOCOL_H_INCLUDE_GUARD
#define SDIR_TOOL_PROTOCOL_H_INCLUDE_GUARD

#include "limit.h"


//--------------------------------------------------------------------------------------------------
/// Name used for both client and server interfaces of the 'sdir' tool protocol.
//--------------------------------------------------------------------------------------------------
#define LE_SDTP_INTERFACE_NAME  "sdirTool"


//--------------------------------------------------------------------------------------------------
/// Protocol ID of the 'sdir' tool protocol.
//--------------------------------------------------------------------------------------------------
#define LE_SDTP_PROTOCOL_ID     "sdirTool"


//--------------------------------------------------------------------------------------------------
/**
 * Message type IDs.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_SDTP_MSGID_LIST,             ///< List all contents of the Service Directory.
                                    ///  Payload is a file descriptor to which output
                                    ///  should be written.

    LE_SDTP_MSGID_LIST_JSON,        ///< Same as LE_SDTP_MSGID_LIST, but the output in json format.

    LE_SDTP_MSGID_UNBIND_ALL,       ///< Delete all bindings (This message has no payload).

    LE_SDTP_MSGID_BIND,             ///< Create one binding.  The payload is the binding details.
                                    ///  If the Service Directory runs into an error, it will
                                    ///  drop the connection to the sdir tool without responding.

    LE_SDTP_MSGID_FIND_SERVICE,     ///< Get the service information by service's user and server
                                    ///  interface name.
    LE_SDTP_MSGID_SUBSCRIBE_EVENT,  ///< Subscribe service availability events.
    LE_SDTP_MSGID_SERVICE_AVAIL,    ///< Event to notify the service is available.
    LE_SDTP_MSGID_SERVICE_UNAVAIL,  ///< Event to notify the service is unavailable.
}
le_sdtp_MsgType_t;


//--------------------------------------------------------------------------------------------------
/**
 * Message structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sdtp_MsgType_t msgType;  ///< Indicates what type of message this is.
    uid_t client;               ///< Unix user ID of the client.
    uid_t server;               ///< Unix user ID of the server.
    char clientInterfaceName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES]; ///< Client's interface name.
    char serverInterfaceName[LIMIT_MAX_IPC_INTERFACE_NAME_BYTES]; ///< Server's interface name.
}
le_sdtp_Msg_t;


//--------------------------------------------------------------------------------------------------
/**
 * The service information response, which contains the server protocol ID and max payload size.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_result_t result;                     ///< result of response.
    char id[LIMIT_MAX_PROTOCOL_ID_BYTES];   ///< Unique identifier for the protocol of the service.
    size_t maxPayloadSize;                  ///< Max payload size (in bytes) in this protocol.
}
le_sdtp_resp_t;

#endif // SDIR_TOOL_PROTOCOL_H_INCLUDE_GUARD

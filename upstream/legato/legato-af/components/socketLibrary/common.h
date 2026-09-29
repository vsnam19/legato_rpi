/**
 * @file common.h
 *

 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef LE_SOCKET_COMMON_H
#define LE_SOCKET_COMMON_H

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of supported socket types
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TCP_TYPE = 0,
    UDP_TYPE,
    UNKNOWN_TYPE
}
SocketType_t;

#endif /* LE_SOCKET_COMMON_H */

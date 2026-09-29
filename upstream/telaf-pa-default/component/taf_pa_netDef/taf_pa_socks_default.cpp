/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "taf_pa_socks.hpp"

/* Implementation */

pa_result_t taf_pa_socks_Init()
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set device mode
 *
 * @return PA_FAULT                      Failed
 *         PA_BAD_PARAMETER              Invalid deviceMode
 *         PA_OK                         Succeeded
 *
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_SetDeviceMode
(
    taf_pa_net_DeviceMode_t deviceMode  ///< [IN] Device mode
)
{
    PA_INFO("Default platform adatper implementation taf_pa_net_SetDeviceMode");
    return PA_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get device mode
 *
 * @return taf_net_DeviceMode_t          Device mode
 *
 */
//--------------------------------------------------------------------------------------------------
taf_pa_net_DeviceMode_t taf_pa_net_GetDeviceMode
(
)
{
    return TAF_PA_NET_DEVICE_NONE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set SOCKS authentication method
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_SetSocksAuthMethod
(
    taf_pa_net_AuthMethod_t authMethod
)
{
    PA_INFO("Default platform adatper implementation taf_pa_net_SetSocksAuthMethod");
    return PA_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get SOCKS authentication method
 */
//--------------------------------------------------------------------------------------------------
taf_pa_net_AuthMethod_t taf_pa_net_GetSocksAuthMethod
(
)
{
    PA_INFO("Default platform adatper implementation taf_pa_net_GetSocksAuthMethod");
    return TAF_PA_NET_SOCKS_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets SOCKS LAN interface
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_SetSocksLanInterface
(
    const char* ifName
)
{
    PA_INFO("Default platform adatper implementation taf_pa_net_SetSocksLanInterface");
    return PA_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets SOCKS LAN interface
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_GetSocksLanInterface
(
    char* ifName,
    size_t ifNameSize
)
{
    PA_INFO("Default platform adatper implementation taf_pa_net_GetSocksLanInterface");
    return PA_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds username/profile association
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_AddSocksAssociation
(
    const char* userName,
    uint32_t profileId
)
{
    PA_INFO("Default platform adatper implementation taf_pa_net_AddSocksAssociation");
    return PA_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes username/profile association
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_RemoveSocksAssociation
(
    const char* userName
)
{
    PA_INFO("Default platform adatper implementation taf_pa_net_RemoveSocksAssociation");
    return PA_OK;
}

/*======================================================================

 FUNCTION        taf_Socks::EnableSocksCmdSync

======================================================================*/
pa_result_t taf_pa_net_EnableSocksCmdSync()
{
    PA_INFO("Default taf_pa_net_EnableSocksCmdSync implementation");
    return PA_NOT_IMPLEMENTED;
}

/*======================================================================

 FUNCTION        taf_Socks::DisableSocksCmdSync

======================================================================*/
pa_result_t taf_pa_net_DisableSocksCmdSync()
{
    PA_INFO("Default taf_pa_net_DisableSocksCmdSync implementation");
    return PA_NOT_IMPLEMENTED;
}
/*======================================================================

 FUNCTION        EnableSocksCmdSync

======================================================================*/
pa_result_t taf_pa_net_EnableSocksCmdASync(taf_pa_socks_CallCb callback,void *contextPtr)
{
    PA_INFO("Default taf_pa_net_EnableSocksCmdASync implementation");
    return PA_NOT_IMPLEMENTED;
}


/*======================================================================

 FUNCTION        DisableSocksCmdSync


======================================================================*/
pa_result_t taf_pa_net_DisableSocksCmdASync(taf_pa_socks_CallCb callback,void *contextPtr)
{
    PA_INFO("Default taf_pa_net_DisableSocksCmdASync implementation");
    return PA_NOT_IMPLEMENTED;
}





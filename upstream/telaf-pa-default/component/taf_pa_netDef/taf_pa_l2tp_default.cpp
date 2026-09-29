/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "taf_pa_l2tp.hpp"

/* Implementation */

bool taf_pa_l2tp_Init()
{
    PA_DEBUG("Enter taf_pa_l2tp_Init in Default PA");
    PA_INFO("Default platform adatper implementation");
    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add Tunnel Asynchronously
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_AddTunnelAsync
(
    const taf_pa_net_L2tpTunnel_t& addTunnelConfig,   // IN
    taf_pa_l2tp_CallCb callback,           // IN
    void* contextPtr                    // IN
)
{
    PA_DEBUG("Enter taf_pa_net_AddTunnelAsync in Default PA");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add Tunnel Synchronously
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_AddTunnelSync
(
    const taf_pa_net_L2tpTunnel_t& addTunnelConfig   // IN
)
{
    PA_DEBUG("Enter taf_pa_net_AddTunnelSync in Default PA");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove Tunnel Asynchronously
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_RemoveTunnelAsync
(
    const uint32_t tunnelId,   // IN
    taf_pa_l2tp_CallCb callback,          // IN
    void* contextPtr                    // IN
)
{
    PA_DEBUG("Enter taf_pa_net_RemoveTunnelAsync in Default PA");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove Tunnel Synchronously
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_RemoveTunnelSync
(
    const uint32_t tunnelId   // IN
)
{
    PA_DEBUG("Enter taf_pa_net_RemoveTunnelSync in Default PA");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Request L2TP Configuration
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_RequestL2tpConfig
(
    taf_pa_net_L2tpConfig_t& L2tpConfig  // OUT
)
{
    PA_DEBUG("Enter taf_pa_net_RequestL2tpConfig in Default PA");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set L2TP Configuration Asynchronously
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_SetL2tpConfigAsync
(
    const taf_pa_net_L2tpConfig_t& L2tpConfig,  // IN
    taf_pa_l2tp_CallCb callback,        // IN
    void* contextPtr                                   // IN
)
{
    PA_DEBUG("Enter taf_pa_net_SetL2tpConfigAsync in Default PA");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set L2TP Configuration Synchronously
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_SetL2tpConfigSync
(
    taf_pa_net_L2tpConfig_t& L2tpConfig  // IN
)
{
    PA_DEBUG("Enter taf_pa_net_SetL2tpConfigSync in Default PA");
    return PA_NOT_IMPLEMENTED;
}

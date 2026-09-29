/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "taf_pa_time.hpp"


//--------------------------------------------------------------------------------------------------
/**
 * PA interface for GNSS
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_gnss_Init(void)
{
    PA_INFO("Default platform adatper PA_NOT_IMPLEMENTED");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_RegGnssTimeListener(void)
{
    PA_INFO("Default platform adatper PA_NOT_IMPLEMENTED");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_DeregGnssTimeListener(void)
{
    PA_INFO("Default platform adatper PA_NOT_IMPLEMENTED");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_time_RegGnssUtcTimeUpdateHandler
(
    taf_pa_time_GnssUtcTimeUpdateHandler_t handlerFunc
)
{
    PA_INFO("Default platform adatper PA_NOT_IMPLEMENTED");
    return PA_NOT_IMPLEMENTED;
}


//--------------------------------------------------------------------------------------------------
/**
 * PA interface for network
 */
//--------------------------------------------------------------------------------------------------

pa_result_t taf_pa_network_Init(int slotId)
{
    PA_INFO("Default platform adatper PA_NOT_IMPLEMENTED");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_time_RegNetworkTimeListener(int slotId)
{
    PA_INFO("Default platform adatper PA_NOT_IMPLEMENTED");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_time_DeregNetworkTimeListener(int slotId)
{
    PA_INFO("Default platform adatper PA_NOT_IMPLEMENTED");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_time_RequestNetworkTime
(
    int slotId,
    taf_pa_time_NetworkInfoHandler_t handlerFunc
)
{
    PA_INFO("Default platform adatper PA_NOT_IMPLEMENTED");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_time_RegNetworkTimeChangeHandler
(
    taf_pa_time_NetworkChangeHandler_t handlerFunc
)
{
    PA_INFO("Default platform adatper PA_NOT_IMPLEMENTED");
    return PA_NOT_IMPLEMENTED;
}


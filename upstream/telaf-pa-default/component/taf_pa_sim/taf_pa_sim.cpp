/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "taf_pa_sim.hpp"

//--------------------------------------------------------------------------------------------------
/**
 *  SIM refresh register.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_sim_RefreshRegister
(
    taf_pa_sim_SessionType_t sessionType,
    uint32_t filesLen,
    taf_pa_sim_RefreshFile_t* files
)
{
    PA_INFO("Function is not implemented in stub PA.");
    return TAF_PA_SIM_RESULT_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 *  SIM refresh ok.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_sim_RefreshOk
(
    taf_pa_sim_SessionType_t sessionType,
    bool* refreshAllow
)
{
    PA_INFO("Function is not implemented in stub PA.");
    return TAF_PA_SIM_RESULT_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 *  SIM refresh complete.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_sim_RefreshComplete
(
    taf_pa_sim_SessionType_t sessionType
)
{   PA_INFO("Function is not implemented in stub PA.");
    return TAF_PA_SIM_RESULT_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler for SIM refresh
 */
//--------------------------------------------------------------------------------------------------
taf_pa_sim_RefreshChangeHandlerRef_t taf_pa_sim_AddRefreshChangeHandler
(
    taf_pa_sim_RefreshChangeHandlerFunc_t handlerFuncPtr,
    void* contextPtr
)
{
	PA_INFO("Function is not implemented in stub PA.");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler for SIM refresh
 */
//--------------------------------------------------------------------------------------------------
void taf_pa_sim_RemoveRefreshChangeHandler
(
    taf_pa_sim_RefreshChangeHandlerRef_t handlerRef ///< [IN] Handler reference.
)
{
    PA_INFO("Function is not implemented in stub PA.");
    return;
}
//--------------------------------------------------------------------------------------------------
/**
 * Initialize PA SIM
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_sim_Init
(

)
{
    PA_INFO("Function is not implemented in stub PA");
    return TAF_PA_SIM_RESULT_NOT_IMPLEMENTED;
}
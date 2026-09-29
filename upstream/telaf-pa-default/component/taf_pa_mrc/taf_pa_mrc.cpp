/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <errno.h>

#include "taf_pa_mrc.hpp"

pa_result_t taf_pa_mrc_Init
(
    void
)
{
    PA_INFO("Function is not implemented in default PA.");

    return -ENOSYS;
}

pa_result_t taf_pa_mrc_RegisterIndication
(
    uint8_t registration
)
{
    PA_INFO("Function is not implemented in default PA.");

    return -ENOSYS;
}

pa_result_t taf_pa_mrc_SetProcessStatus
(
    taf_pa_mrc_Process_t process,
    taf_pa_mrc_Status_t status
)
{
    PA_INFO("Function is not implemented in default PA.");

    return -ENOSYS;
}

taf_pa_mrc_ProcessStatusHandlerRef_t taf_pa_mrc_AddProcessStatusHandler
(
    taf_pa_mrc_ProcessStatusHdlrFunc_t handlerFuncPtr,
    void* contextPtr
)
{
    PA_INFO("Function is not implemented in default PA.");

    return nullptr;
}

pa_result_t taf_pa_mrc_GetEfsPeStatus
(
    taf_pa_mrc_EfsPeStatus_t* statusPtr
)
{
    PA_INFO("Function is not implemented in default PA.");

    return -ENOSYS;
}

pa_result_t taf_pa_mrc_GetEfsBlockStatus
(
    taf_pa_mrc_EfsBlockStatus_t* statusPtr
)
{
    PA_INFO("Function is not implemented in default PA.");

    return -ENOSYS;
}

pa_result_t taf_pa_mrc_SetTimerPeriod
(
    taf_pa_mrc_Timer_t timer,
    uint32_t period
)
{
    PA_INFO("Function is not implemented in default PA.");

    return -ENOSYS;
}

pa_result_t taf_pa_mrc_PerformABSync
(
    void
)
{
    PA_INFO("Function is not implemented in default PA.");

    return -ENOSYS;
}
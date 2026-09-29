/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <future>
#include <unistd.h>
#include <any>
#include "taf_pa_common.h"
#include "taf_pa_voicecall.hpp"

/* Implementation */
using namespace tafpa::voicecall;

pa_result_t tafpa::voicecall::taf_pa_voicecall_Init()
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::voicecall::taf_pa_voicecall_Make
(
    const taf_pa_voicecall_CallInfo_t& callInfo,
    taf_pa_voicecall_CallCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::voicecall::taf_pa_voicecall_Stop
(
    const taf_pa_voicecall_CallInfo_t& callInfo,
    taf_pa_voicecall_CallCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::voicecall::taf_pa_voicecall_Hold
(
    const taf_pa_voicecall_CallInfo_t& callInfo,
    taf_pa_voicecall_CallCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::voicecall::taf_pa_voicecall_Resume
(
    const taf_pa_voicecall_CallInfo_t& callInfo,
    taf_pa_voicecall_CallCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::voicecall::taf_pa_voicecall_Answer
(
    const taf_pa_voicecall_CallInfo_t& callInfo,
    taf_pa_voicecall_CallCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::voicecall::taf_pa_voicecall_Swap
(
    const taf_pa_voicecall_CallInfo_t& callInfo,
    taf_pa_voicecall_CallCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::voicecall::taf_pa_voicecall_RegisterEventListener
(
    taf_pa_voicecall_EventListener listener, std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::voicecall::taf_pa_voicecall_GetCallTermination
(
    const taf_pa_voicecall_CallInfo_t& callInfo,
    taf_pa_voicecall_termination_t *termination
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}


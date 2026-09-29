/*
*  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
*  SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include <chrono>
#include <future>
#include <unistd.h>
#include <any>
#include "taf_pa_ecall.hpp"

/* Implementation */
using namespace tafpa::ecall;

// Macro to suppress unused parameter warnings
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

pa_result_t tafpa::ecall::taf_pa_ecall_Init()
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_SetConfig(
    const taf_pa_ecall_config_t& config)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(config);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_GetConfig(
    taf_pa_ecall_config_t& config
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(config);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_SetOpMode(
    uint8_t phoneId,
    taf_pa_ecall_mode_t mode,
    taf_pa_ecall_CommandCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(phoneId);
    UNUSED(mode);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_GetOpMode(
    uint8_t phoneId,
    taf_pa_ecall_GetModeCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(phoneId);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

std::vector<std::shared_ptr<taf_pa_ecall_CallInfo_t>> tafpa::ecall::taf_pa_ecall_GetInProgressCalls()
{
    PA_INFO("Using default PA. Feature unsupported.");
    return {};
}

pa_result_t tafpa::ecall::taf_pa_ecall_RequestNetworkDeregistration(
    uint8_t phoneId,
    taf_pa_ecall_CommandCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(phoneId);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_MakeECall(
    int phoneId,
    const taf_pa_ecall_msd_data_t& msdData,
    taf_pa_ecall_category_t category,
    taf_pa_ecall_type_t type,
    taf_pa_ecall_MakeEcallCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(phoneId);
    UNUSED(msdData);
    UNUSED(category);
    UNUSED(type);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_MakeECall(
    int phoneId,
    const std::vector<uint8_t>& msdPdu,
    taf_pa_ecall_category_t category,
    taf_pa_ecall_type_t type,
    taf_pa_ecall_MakeEcallCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(phoneId);
    UNUSED(msdPdu);
    UNUSED(category);
    UNUSED(type);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_MakeECall(
    int phoneId,
    std::string dialNumber,
    const taf_pa_ecall_custom_sip_header_t& header,
    const std::vector<uint8_t>& msdPdu,
    taf_pa_ecall_MakeEcallCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(phoneId);
    UNUSED(dialNumber);
    UNUSED(header);
    UNUSED(msdPdu);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_UpdateMsd(
    uint8_t phoneId,
    const taf_pa_ecall_msd_data_t& msdData,
    taf_pa_ecall_CommandCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(phoneId);
    UNUSED(msdData);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_UpdateMsd(
    uint8_t phoneId,
    const std::vector<uint8_t>& msdData,
    taf_pa_ecall_CommandCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(phoneId);
    UNUSED(msdData);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_EncodeMsd(
    const taf_pa_ecall_msd_data_t& msdData,
    std::vector<uint8_t>& msdPdu
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(msdData);
    UNUSED(msdPdu);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_RequestHlapTimerStatus(
    int phoneId,
    taf_pa_ecall_HlapTimerStatusCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(phoneId);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_RequestHlapTimer(
    int phoneId,
    taf_pa_ecall_hlap_timer_type_t type,
    taf_pa_ecall_HlapTimerCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(phoneId);
    UNUSED(type);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_UpdateHlapTimer(
    int phoneId,
    taf_pa_ecall_hlap_timer_type_t type,
    uint32_t duration,
    taf_pa_ecall_CommandCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(phoneId);
    UNUSED(type);
    UNUSED(duration);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_SetEcallRedial(
    const std::vector<int>& timeGap,
    taf_pa_ecall_CommandCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(timeGap);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_GetEcallRedial(
    std::vector<int>& callOrigTimeGap,
    std::vector<int>& callDropTimeGap
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(callOrigTimeGap);
    UNUSED(callDropTimeGap);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_RestartHlapTimer(
    int phoneId,
    taf_pa_ecall_hlap_timer_id_t id,
    uint32_t duration,
    taf_pa_ecall_CommandCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(phoneId);
    UNUSED(id);
    UNUSED(duration);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

int8_t tafpa::ecall::taf_pa_ecall_GetPhoneIdFromSlotId(
    int8_t slotId
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(slotId);
    return -1; // Return invalid phone ID
}

pa_result_t tafpa::ecall::taf_pa_ecall_Hangup(
    const taf_pa_ecall_CallInfo_t& callInfo,
    taf_pa_ecall_CommandCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(callInfo);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_Reject(
    const taf_pa_ecall_CallInfo_t& callInfo,
    taf_pa_ecall_CommandCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(callInfo);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_Answer(
    const taf_pa_ecall_CallInfo_t& callInfo,
    taf_pa_ecall_CommandCb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(callInfo);
    UNUSED(callback);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::ecall::taf_pa_ecall_RegisterListener(
    const taf_pa_ecall_event_listener_t* eventListener,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    UNUSED(eventListener);
    UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}
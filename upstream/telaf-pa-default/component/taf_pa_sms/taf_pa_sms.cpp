/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "taf_pa_common.h"
#include "taf_pa_sms.hpp"

using namespace tafpa::sms;

pa_result_t tafpa::sms::taf_pa_sms_Init
(
    void
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sms::taf_pa_sms_ReadMessage
(
    uint32_t readAtIdx,
    uint32_t timeout,
    uint8_t phoneId,
    taf_pa_sms_Tag* pduRxStatus,
    std::vector<uint8_t>& pduBuffer,
    uint32_t* pduMsgIndex
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

void tafpa::sms::taf_pa_sms_RegisterIncomingSmsCallback
(
    IncomingSmsCallback cb
)
{
    PA_INFO("Using default PA. Feature unsupported.");
}

void tafpa::sms::taf_pa_sms_RegisterMemoryFullCallback
(
    MemoryFullCallback cb
)
{
    PA_INFO("Using default PA. Feature unsupported.");
}

pa_result_t tafpa::sms::taf_pa_sms_SendRawSms
(
    uint8_t* pduData,
    uint32_t pduLength,
    uint32_t timeout,
    uint8_t phoneId
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

void tafpa::sms::taf_pa_sms_SendPDUMessageAsync
(
    uint8_t phoneId,
    const uint8_t* pduData,
    size_t pduLength,
    std::function<void(pa_result_t)> cb
)
{
    PA_INFO("Using default PA. Feature unsupported.");
}

pa_result_t tafpa::sms::taf_pa_sms_SetActivationStatus
(
    uint8_t phoneId,
    bool activate,
    uint32_t timeout
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sms::taf_pa_sms_RequestMessageFilters
(
    uint8_t phoneId,
    uint32_t timeout
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sms::taf_pa_sms_AddCellBroadcastIds
(
    uint8_t phoneId,
    uint16_t fromId,
    uint16_t toId,
    uint32_t timeout
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sms::taf_pa_sms_RemoveCellBroadcastIds
(
    uint8_t phoneId,
    uint16_t fromId,
    uint16_t toId,
    uint32_t timeout
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sms::taf_pa_sms_GetSmsCenterAddress
(
    uint8_t phoneId,
    char* addr,
    size_t len,
    uint32_t timeout
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sms::taf_pa_sms_SetSmsCenterAddress
(
    uint8_t phoneId,
    const char* addr,
    uint32_t timeout
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sms::taf_pa_sms_GetPreferredStorage
(
    taf_pa_sms_Storage* type,
    uint32_t timeout,
    uint8_t phoneId
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sms::taf_pa_sms_SetPreferredStorage
(
    taf_pa_sms_Storage type,
    uint32_t timeout,
    uint8_t phoneId
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sms::taf_pa_sms_SetTag
(
    uint32_t msgIndex,
    taf_pa_sms_Tag tagType,
    uint32_t timeout,
    uint8_t phoneId
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sms::taf_pa_sms_DeleteMessage
(
    uint32_t msgIndex,
    uint32_t timeout,
    uint8_t phoneId
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

int32_t tafpa::sms::taf_pa_sms_RequestSmsMessageList
(
    uint32_t* arr,
    size_t arrSize,
    uint32_t timeout,
    taf_pa_sms_Tag tagType,
    uint8_t phoneId
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return 0;
}

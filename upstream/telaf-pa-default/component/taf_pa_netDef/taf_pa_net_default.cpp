/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "taf_pa_net.hpp"

/* Implementation */

pa_result_t taf_pa_net_Init()
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get phone ID from slot ID
 *
 * @return PA_OK            Success
 *         PA_BAD_PARAMETER Invalid parameter
 *         PA_FAULT         Failure
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_GetPhoneIdFromSlotId
(
    uint8_t slotId,      ///< [IN] Slot ID
    uint8_t *phoneIdPtr  ///< [OUT] Phone ID
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get slot ID from phone ID
 *
 * @return PA_OK            Success
 *         PA_BAD_PARAMETER Invalid parameter
 *         PA_FAULT         Failure
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_net_GetSlotIdFromPhoneId
(
    uint8_t phoneId,     ///< [IN] Phone ID
    uint8_t *slotIdPtr   ///< [OUT] Slot ID
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

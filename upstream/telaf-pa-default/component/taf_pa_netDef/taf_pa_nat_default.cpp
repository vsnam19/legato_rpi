/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "taf_pa_nat.hpp"

/* Implementation */

pa_result_t taf_pa_nat_Init()
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add a destination NAT entry
 *
 * @return
 *      - PA_OK on success
 *      - PA_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
 pa_result_t taf_pa_nat_AddDestNatEntry(
    uint32_t profileId,
    uint8_t slotId,
    const taf_pa_net_NatConfig_t *natConfig
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove a destination NAT entry
 *
 * @return
 *      - PA_OK on success
 *      - PA_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
 pa_result_t taf_pa_nat_RemoveDestNatEntry(
    uint32_t profileId,
    uint8_t slotId,
    const taf_pa_net_NatConfig_t *natConfig
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Query destination NAT entry list for a profile
 *
 * @return
 *      - PA_OK on success
 *      - PA_FAULT on failure
 *      - PA_TIMEOUT on timeout
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_nat_QueryDestNatEntryList(
    uint32_t profileId,
    uint8_t slotId,
    std::vector<taf_pa_net_NatConfig_t> &natEntryInfo
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}


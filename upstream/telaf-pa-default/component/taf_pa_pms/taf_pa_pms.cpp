/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "taf_pa_pms.hpp"

/* ------------------------------------------------------ */
/* ------------------------------------------------------ */
/* -------------------( PaFn - Fcall ) ------------------ */
/* ------------------------------------------------------ */
/* ------------------------------------------------------ */

PaType(Result) PaFn(Init)
(
    PaType(Reference)  *paRefPtr,
    SendEventFunc_t     fnSendEvent,
    uint32_t            timeoutMs
)
{
    PA_INFO("[DEF PA] NOT implemented");
    return PaResult(NOT_IMPLEMENTED);
}

void PaFn(Deinit)
(
    PaType(Reference) *paRefPtr
)
{
    PA_INFO("[DEF PA] NOT implemented");
}

PaType(Result) PaFn(SetPowerStateAsMaster)
(
    PaType(Reference)         paRef,
    PaType(PowerState)        state,
    const char               *name
)
{
    PA_INFO("[DEF PA] NOT implemented");
    return PaResult(NOT_IMPLEMENTED);
}

PaType(Result) PaFn(SendAckForStateUpdate)
(
    PaType(Reference)  paRef,
    PaType(PowerState) state,
    PaType(Ack)        ack
)
{
    PA_INFO("[DEF PA] NOT implemented");
    return PaResult(NOT_IMPLEMENTED);
}

PaType(Result) PaFn(GetAllMachineNames)
(
    PaType(Reference)  paRef,
    std::vector<std::string> & machineNames
)
{
    PA_INFO("[DEF PA] NOT implemented");
    return PaResult(NOT_IMPLEMENTED);
}

PaType(Result) PaFn(SetModemWakeupFilter)
(
    PaType(Reference)         paRef,
    uint32_t                  wsBitmask
)
{
    PA_INFO("[DEF PA] NOT implemented");
    return PaResult(NOT_IMPLEMENTED);
}

PaType(Result) PaFn(GetModemWakeupFilter)
(
    PaType(Reference)         paRef,
    uint32_t                 *wsBitmaskPtr
)
{
    PA_INFO("[DEF PA] NOT implemented");
    return PaResult(NOT_IMPLEMENTED);
}

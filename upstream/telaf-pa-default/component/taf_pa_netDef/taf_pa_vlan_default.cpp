/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "taf_pa_vlan.hpp"
#include <vector>

/* Implementation */

pa_result_t taf_pa_vlan_Init()
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register hardware acceleration notification handler in PA layer
 */
//--------------------------------------------------------------------------------------------------
pa_result_t taf_pa_vlan_RegHwAccelarationUpdateHandler
(
    taf_pa_vlan_HardwareAccelerationHandler_t handlerFunc
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_AddVlanInterface
(
     const taf_pa_Vlan_t vlanConfig,
     const taf_pa_vlan_iface_type_t iftype
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_RemoveVlanInterface
(
     const taf_pa_Vlan_t vlanConfig,
     const taf_pa_vlan_iface_type_t iftype
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_BindWithProfile
(
     const taf_pa_Vlan_t vlan,
     const taf_pa_VlanBindConfig_t vlanBindConfig
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_UnbindWithProfile
(
     const taf_pa_Vlan_t vlan,
     const taf_pa_VlanBindConfig_t vlanBindConfig
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_BindWithBackhaul
(
     const taf_pa_Vlan_t vlan,
     const taf_pa_VlanBindConfig_t vlanBindConfig
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_UnbindWithBackhaul
(
     const taf_pa_Vlan_t vlan,
     const taf_pa_VlanBindConfig_t vlanBindConfig
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_QueryVlanInfo
(
    std::vector<taf_pa_Vlan_t> &vlanEntryInfo    // OUT
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_QueryVlanMappingList
(
     const uint8_t slotId,                     // IN
     std::list<std::pair<int, int>> &mapping   // OUT
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_QueryVlanToBackhaulMappingList
(
    const uint8_t slotID,                          // IN
    const taf_pa_vlan_backhaul_type_t backhaulType,  // IN
    std::vector<taf_pa_VlanBindConfig_t>& vlanEntryInfo  // OUT
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_RegVlanListener()
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_DeregVlanListener()
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

//Data Settings API

pa_result_t taf_pa_net_GetBackhaulPreference
(
    std::vector<taf_pa_vlan_backhaul_type_t> &backhaulPref    // OUT
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_SetBackhaulPreference
(
    std::vector<taf_pa_vlan_backhaul_type_t> backhaulPref    // IN
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_GetIPPassThroughNatConfig
(
    bool &isEnabledPtr                         // OUT
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_SetIPPassThroughNatConfig
(
    bool isEnabledPtr                         // IN
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_SetIPPassThroughConfig
(
    const taf_pa_IpptConfigIn_t *ipptConfigIn,    // IN
    const taf_pa_IpptConfigOut_t *ipptConfigOut   // IN
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_GetIPPassThroughConfig
(
    const taf_pa_IpptConfigIn_t *ipptConfigIn,    // IN
    taf_pa_IpptConfigOut_t *ipptConfigOut         // OUT
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_SetIPConfig
(
    const taf_pa_IpConfigParams_t *ipConfigParams,  // IN
    const taf_pa_IpConfig_t *ipConfig               // IN
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t taf_pa_net_GetIPConfig
(
    const taf_pa_IpConfigParams_t *ipConfigParams,  // IN
    taf_pa_IpConfig_t *ipConfig                     // OUT
)
{
    PA_INFO("Default platform adatper implementation");
    return PA_NOT_IMPLEMENTED;
}

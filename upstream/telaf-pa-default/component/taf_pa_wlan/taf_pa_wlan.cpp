/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "taf_pa_wlan.hpp"
#include "taf_pa_common.h"

namespace taf::pa::wlan {

pa_result_t Init()
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t RegisterDeviceListener(DeviceListener, std::any)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t EnableDevice(bool)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t GetStatus(bool&)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t SetDeviceMode(int, int)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t GetDeviceMode(int&, int&)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t SetStaBridgeMode(StaId_e, Mode_e)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t GetStaBridgeMode(StaId_e, Mode_e&)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t SetStaIpConfig(StaId_e, IPType_e)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t SetStaIpConfig(StaId_e, IPType_e, const StaIpConfig_t&)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t GetStaIpConfig(StaId_e, IPType_e&, StaIpConfig_t&)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t GetBandInterferenceConfig(bool&, BandInterferenceConfig_t&)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t SetBandInterferenceConfig(bool, const BandInterferenceConfig_t&)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

} // namespace taf::pa::wlan

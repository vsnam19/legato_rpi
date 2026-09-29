/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "taf_pa_audio.hpp"

/* Implementation */
using namespace tafpa::audio;

pa_result_t tafpa::audio::taf_pa_audio_Init()
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_CreateStream(
    PaStreamConfig streamConfig,
    taf_pa_audio_cb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_DeleteStream(
    PaStreamConfig streamConfig,
    taf_pa_audio_cb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_StartAudio(
    PaStreamConfig streamConfig,
    taf_pa_audio_cb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_StopAudio(
    PaStreamConfig streamConfig,
    taf_pa_audio_cb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

// pa_result_t tafpa::audio::taf_pa_audio_StopVoiceStream(
//     taf_pa_audio_RouteId_t routeId
// )
// {
//     PA_INFO("Using default PA. Feature unsupported.");
//     return PA_NOT_IMPLEMENTED;
// }

pa_result_t tafpa::audio::taf_pa_audio_SetVolume(
    PaStreamConfig streamConfig,
    double volLevel,
    taf_pa_audio_cb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_GetVolume(
    PaStreamConfig streamConfig,
    double *volLevel,
    taf_pa_audio_cb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_SetMute(
    PaStreamConfig streamConfig,
    bool isMute,
    taf_pa_audio_cb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_GetMute(
    PaStreamConfig streamConfig,
    bool *isMute,
    taf_pa_audio_cb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_StartPlayback(
    std::vector<taf_pa_audio_PlayFileInfo_t> &playFileInfos,
    int listSiz,
    std::weak_ptr<IPaPlayListListener> pbStatusListener
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_StopPlayback(
    PaStreamConfig streamConfig
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

std::shared_ptr<PaAudioCaptureStream> tafpa::audio::taf_pa_audio_GetCaptureStream(
    PaStreamDirection streamDir
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return NULL;
}

pa_result_t tafpa::audio::taf_pa_audio_PlaySignallingDtmfOnTx(
    uint32_t slotId,
    const char dtmf,
    taf_pa_audio_cb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_StopSignallingDtmfOnTx(
    uint32_t slotId,
    taf_pa_audio_cb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_PlayDtmf(
        PaDtmfTone dtmfTone, uint16_t duration,
        uint16_t gain,
        taf_pa_audio_cb callback,
        std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_StopDtmf(
    PaStreamDirection direction, taf_pa_audio_cb callback,
    std::any context
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_registerDtmfListener(
    std::weak_ptr<IPaDtmfListener> dtmfListener
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::audio::taf_pa_audio_deregisterDtmfListener(
    std::weak_ptr<IPaDtmfListener> dtmfListener
)
{
    PA_INFO("Using default PA. Feature unsupported.");
    return PA_NOT_IMPLEMENTED;
}

/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file taf_pa_dataDef.cpp
 * @brief The APIs implementation interface. From this file the underlying APIs will be called to
 *        perform the required operations.
 *
 * The external APIs call a "weak" API which will be implemented in the actual PA.
 *
 */

#include "taf_pa_data.hpp"

#define PA_UNUSED(x) (void)(x)

//--------------------------------------------------------------------------------------------------
/**
 * Get the Telux data PA state.
 *
 * @return
 *  - PA_OK              PA completely initialized
 *  - PA_UNAVAILABLE     PA not completely initialized. A part of the PA maybe usable. Check state.
 *  - PA_FAULT           PA is not usable due to fatal failure.
 *  - PA_NOT_IMPLEMENTED API is not implemented.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::Init
(
    SubsystemState_e &state
        ///< [OUT] The Telux data PA initialization state.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(state);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deinitialize the Telux data PA state.
 *
 * @return
 *  - PA_OK              PA completely initialized
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::Deinit()
{
    PA_DEBUG("Default PA implementation.");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Telux data PA initialization state.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::GetSubsystemState
(
    taf::pa::data::PhoneId_e phoneId,
    ///< [IN] The phone ID.
    taf::pa::data::Subsystem_e subsystem,
    ///< [IN] The subsystem.
    taf::pa::data::SubsystemState_e &state
    ///< [OUT] The subsystem initialization state.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(phoneId);
    PA_UNUSED(subsystem);
    PA_UNUSED(state);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the phone Ids.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::GetPhoneIds
(
    std::vector<taf::pa::data::PhoneId_e> &phoneIds
        ///< [OUT] The phone IDs.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(phoneIds);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM slot count.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::GetSimSlotCount
(
    taf::pa::data::SlotCount_e &slotCount
        ///< [OUT] The number of SIM slots.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(slotCount);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM slot count.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::GetPhoneIdFromSimSlotId
(
    taf::pa::data::SlotId_e slotID,
        ///< [IN] The SIM slot ID.
    taf::pa::data::PhoneId_e& phoneID
        ///< [OUT] The phone ID.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(slotID);
    PA_UNUSED(phoneID);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM slot count.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::GetSimSlotIdFromPhoneId
(
    taf::pa::data::PhoneId_e phoneID,
        ///< [IN] The phone ID.
    taf::pa::data::SlotId_e& slotID
        ///< [OUT] The SIM slot ID.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(phoneID);
    PA_UNUSED(slotID);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get profiles from the NAD for the specified slot ID.
 *
 * Use context to distinguish between different slots if the same handler is used.
 *
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::GetProfilesAsync
(
    taf::pa::data::PhoneId_e phoneId,
    taf_pa_data_profile_GetAllAsyncCb callback,
    void* contextPtr
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(phoneId);
    PA_UNUSED(callback);
    PA_UNUSED(contextPtr);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get details of the specified profile.
 *
 * Set ProfileInfo_t::profileId to the desired profile ID.
 * Set ProfileInfo_t::techPref to 3GPP or 3GPP2. If unspecified, 3GPP will be used.
 *
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::GetProfileInfo
(
    PhoneId_e phoneId,
    ///< [IN] The phone id.
    ProfileInfo_t &profileInfo
    ///< [IN/OUT] The profile information.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(phoneId);
    PA_UNUSED(profileInfo);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a profile
 *
 * On success, the created profile ID will be available.
 *
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::CreateProfile
(
    taf::pa::data::PhoneId_e phoneId,
    ///< [IN] The phone id.
    taf::pa::data::ProfileInfo_t profileInfo,
    ///< [IN] The profile information.
    ProfileId_e &profileId
    ///< [OUT] The profile id on success.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(phoneId);
    PA_UNUSED(profileInfo);
    PA_UNUSED(profileId);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Update a profile
 *
 *
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::UpdateProfile
(
    taf::pa::data::PhoneId_e phoneId,
    ///< [IN] The phone id.
    taf::pa::data::ProfileInfo_t profileInfo
    ///< [IN] The profile information.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(phoneId);
    PA_UNUSED(profileInfo);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Update a profile
 *
 * Only the phone ID and profile ID are considered in the structure.
 *
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::DeleteProfile
(
    taf::pa::data::PhoneId_e phoneId,
    ///< [IN] The phone id.
    taf::pa::data::ProfileInfo_t profileInfo
    ///< [IN] The profile information.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(phoneId);
    PA_UNUSED(profileInfo);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the default profile
 *
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::GetDefaultProfile
(
    taf::pa::data::PhoneId_e phoneId,
    ///< [IN] The profile information.
    taf::pa::data::ProfileId_e &profileId
    ///< [OUT] The default profile ID.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(phoneId);
    PA_UNUSED(profileId);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the default profile
 *
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::SetDefaultProfile
(
    taf::pa::data::PhoneId_e phoneId,
    ///< [IN] The profile information.
    taf::pa::data::ProfileId_e profileId
    ///< [IN] The default profile ID.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(phoneId);
    PA_UNUSED(profileId);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register for data call events callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::AddDataCallEventsCallback
(
    taf_pa_data_CallEventsCb callBack,
        ///< [IN] The callback function.
    std::shared_ptr<void> context,
        ///< [IN] The context pointer.
    uint16_t &id
        ///< [OUT] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(callBack);
    PA_UNUSED(context);
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removed a previously registered data call events callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::RemoveDataCallEventsCallback
(
    uint16_t id
        ///< [IN] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::StartDataSessionAsync
(
    const taf::pa::data::DataCallStartStopParams_t& params ///< [IN] The IP type.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(params);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop a data session
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::StopDataSessionAsync
(
    const taf::pa::data::DataCallStartStopParams_t& params ///< [IN] The IP type.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(params);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Request list of all active data calls. Events will be provided via taf_pa_data_RequestCallListCb.
 *
 * @return LE_OK on success. Wait for callback for final status.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::RequestDataCallsListAsync
(
    PhoneId_e phoneId,
                ///< [IN] The phone ID.
    taf_pa_data_RequestCallListCb callBack,
                ///< [IN] The callback function.
    std::shared_ptr<void> context
                ///< [IN] The context pointer.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(phoneId);
    PA_UNUSED(callBack);
    PA_UNUSED(context);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register roaming events callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::AddRoamingEventsCallback
(
    taf_pa_data_RoamingEventsCb callBack,
        ///< [IN] The callback function.
    std::shared_ptr<void> context,
        ///< [IN] The context pointer.
    uint16_t &id
        ///< [OUT] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(callBack);
    PA_UNUSED(context);
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removed a previously registered roaming events callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::RemoveRoamingEventsCallback
(
    uint16_t id
        ///< [IN] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get roaming status. Events will be provided via taf_pa_data_RoamingEventsCb that is registered
 * via AddRoamingEventsCallback()
 *
 * @return PA_OK on success. Wait for callback for final status.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::GetRoamingStatus
(
    const taf::pa::data::PhoneId_e phoneId,
    RoamingStatus_t &roamingStatus
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(phoneId);
    PA_UNUSED(roamingStatus);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register throttled APN events callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::AddThrottledApnEventsCallback
(
    taf_pa_data_ThrottledApnEventsCb callBack,
        ///< [IN] The callback function.
    std::shared_ptr<void> context,
        ///< [IN] The context pointer.
    uint16_t &id
        ///< [OUT] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(callBack);
    PA_UNUSED(context);
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removed a previously registered throttled APN events callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::RemoveThrottledApnEventsCallback
(
    uint16_t id
        ///< [IN] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get throttled APNs information.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::GetThrottledApnInfo
(
    const taf::pa::data::PhoneId_e phoneId,
        ///< [IN] The phone ID.
    std::vector<ThrottledApnEventInfo_t> &throttledApnEventInfoList
        ///< [OUT] The list of throttled APNs info.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(phoneId);
    PA_UNUSED(throttledApnEventInfoList);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register QoS TFT events callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::AddQosTftEventsCallback
(
    taf_pa_data_QosTftEventsCb callBack,
        ///< [IN] The callback function.
    std::shared_ptr<void> context,
        ///< [IN] The context pointer.
    uint16_t &id
        ///< [OUT] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(callBack);
    PA_UNUSED(context);
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removed a previously registered QoS TFT events callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::RemoveQosTftEventsCallback
(
    uint16_t id
        ///< [IN] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register HW acceleration change events callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::AddHwAccelerationChangeEventsCallback
(
    taf_pa_data_HwAccelerationEventsCb callBack,
    ///< [IN] The callback function.
    std::shared_ptr<void> context,
    ///< [IN] The context pointer.
    uint16_t &id
    ///< [OUT] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(callBack);
    PA_UNUSED(context);
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removed a previously registered HW acceleration change events callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::RemoveHwAccelerationChangeEventsCallback
(
    uint16_t id
    ///< [IN] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register profile change events callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::AddProfileEventsCallback
(
    taf_pa_data_ProfileEventsCb callBack,
        ///< [IN] The callback function.
    std::shared_ptr<void> context,
        ///< [IN] The context pointer.
    uint16_t &id
        ///< [OUT] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(callBack);
    PA_UNUSED(context);
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removed a previously registered profile  events callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::RemoveProfileEventsCallback
(
    uint16_t id
        ///< [IN] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register roaming events callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::AddSubsystemStateChangeCallback
(
    taf_pa_data_SubsystemStateChangeCb callBack,
    ///< [IN] The callback function.
    std::shared_ptr<void> context,
    ///< [IN] The context pointer.
    uint16_t &id
    ///< [OUT] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(callBack);
    PA_UNUSED(context);
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Removed a previously registered subsystem state change callback
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::RemoveSubsystemStateChangeCallback
(
    uint16_t id
        ///< [IN] The ID of the registered callback.
)
{
    PA_DEBUG("Default PA implementation.");
    PA_UNUSED(id);
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register SDK callbacks. This is typically not needed as the callbacks will be registered during
 * initialization.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::RegisterSDKCallbacks()
{
    PA_DEBUG("Default PA implementation.");
    return PA_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deregister SDK callbacks. This is to support the service manage suspend/resume scenarios.
 */
//--------------------------------------------------------------------------------------------------
pa_result_t PA_WEAK taf::pa::data::DeregisterSDKCallbacks()
{
    PA_DEBUG("Default PA implementation.");
    return PA_NOT_IMPLEMENTED;
}

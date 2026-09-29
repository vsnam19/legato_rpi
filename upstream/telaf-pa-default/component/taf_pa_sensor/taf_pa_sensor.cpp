/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "taf_pa_sensor.hpp"

/* Implementation */
using namespace tafpa::sensor;

pa_result_t tafpa::sensor::taf_pa_sensor_Init(int8_t& listSize)
{
    PA_INFO("Using default PA for Sensor. Feature unsupported.");
    listSize = 0;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sensor::taf_pa_sensor_GetSensorInfo(
    int8_t index,
    taf_pa_sensor_BasicInfo &basicInfo,
    taf_pa_sensor_ConfigInfo &configInfo,
    taf_pa_sensor_Capabilities &capabilities
)
{
    PA_INFO("Using default PA for Sensor. Feature unsupported.");
    memset(&basicInfo, 0, sizeof(taf_pa_sensor_BasicInfo));
    basicInfo.sensorType = taf_pa_sensor_SensorType::INVALID;
    memset(&configInfo, 0, sizeof(taf_pa_sensor_ConfigInfo));
    memset(&capabilities, 0, sizeof(taf_pa_sensor_Capabilities));
    return PA_NOT_IMPLEMENTED;
}

taf_pa_sensor_SensorId tafpa::sensor::taf_pa_sensor_GetSensorClient(const std::string& sensorName)
{
    PA_INFO("Using default PA for Sensor. Feature unsupported.");
    (void)sensorName;
    return 0;
}

pa_result_t tafpa::sensor::taf_pa_sensor_ReleaseSensorClient(taf_pa_sensor_SensorId sensorId)
{
    PA_INFO("Using default PA for Sensor. Feature unsupported.");
    (void)sensorId;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sensor::taf_pa_sensor_Activate(
    taf_pa_sensor_SensorId sensorId,
    double sampleRate,
    uint32_t batchCount,
    bool isRotated
)
{
    PA_INFO("Using default PA for Sensor. Feature unsupported.");
    (void)sensorId;
    (void)sampleRate;
    (void)batchCount;
    (void)isRotated;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sensor::taf_pa_sensor_SetEulerAngle(
    taf_pa_sensor_SensorId sensorId,
    double pitch,
    double roll,
    double yaw
)
{
    PA_INFO("Using default PA for Sensor. Feature unsupported.");
    (void)sensorId;
    (void)pitch;
    (void)roll;
    (void)yaw;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sensor::taf_pa_sensor_Deactivate(taf_pa_sensor_SensorId sensorId)
{
    PA_INFO("Using default PA for Sensor. Feature unsupported.");
    (void)sensorId;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sensor::taf_pa_sensor_SelfTest(
    taf_pa_sensor_SensorId sensorId,
    taf_pa_sensor_SelfTestMode mode,
    taf_pa_sensor_SelfTestResultCb callback,
    std::any context
)
{
    PA_INFO("Using default PA for Sensor. Feature unsupported.");
    (void)sensorId;
    (void)mode;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::sensor::taf_pa_sensor_RegisterListener(
    taf_pa_sensor_SensorId sensorId,
    taf_pa_sensor_EventListener* eventListener,
    std::any context
)
{
    PA_INFO("Using default PA for Sensor. Feature unsupported.");
    (void)sensorId;
    (void)eventListener;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}
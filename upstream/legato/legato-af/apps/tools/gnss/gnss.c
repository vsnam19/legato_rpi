//-------------------------------------------------------------------------------------------------
/**
 * @file gnss.c
 *
 * Tool to debug/monitor GNSS device.
 *
 * Copyright (C) Sierra Wireless Inc.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
//-------------------------------------------------------------------------------------------------

#include "gnss.h"

//-------------------------------------------------------------------------------------------------
/**
 * Default time(in second) for 3D fixing after starting gnss device.
 */
//-------------------------------------------------------------------------------------------------
#define DEFAULT_3D_FIX_TIME   60


//-------------------------------------------------------------------------------------------------
/**
 * Default watch period(in second) to get positioning information.
 */
//-------------------------------------------------------------------------------------------------
#define DEFAULT_WATCH_PERIOD   10*60


//-------------------------------------------------------------------------------------------------
/**
 * Max characters for constellations name.
 */
//-------------------------------------------------------------------------------------------------
#define CONSTELLATIONS_NAME_LEN     256


//-------------------------------------------------------------------------------------------------
/**
 * Base 10
 */
//-------------------------------------------------------------------------------------------------
#define BASE10 10

//-------------------------------------------------------------------------------------------------
/**
 * Different type of constellation.
 * {@
 */
//-------------------------------------------------------------------------------------------------
#define CONSTELLATION_DEFAULT       0x1
#define CONSTELLATION_GPS           0x2
#define CONSTELLATION_GLONASS       0x4
#define CONSTELLATION_BEIDOU        0x8
#define CONSTELLATION_GALILEO       0x10
#define CONSTELLATION_SBAS          0x20  //TelSDK supported this constellation
#define CONSTELLATION_QZSS          0x40
#define CONSTELLATION_NAVIC         0x80
// @}

//-------------------------------------------------------------------------------------------------
/**
 * Position handler reference.
 */
//-------------------------------------------------------------------------------------------------
static le_gnss_PositionHandlerRef_t PositionHandlerRef;
static taf_locGnss_CapabilityChangeHandlerRef_t CapabilityHandlerRef;
static taf_locGnss_NmeaHandlerRef_t NmeaHandlerRef;
static taf_locGnss_MeasurementHandlerRef_t MeasHandlerRef;

static le_mutex_Ref_t gnssMutexRef = NULL;

//-------------------------------------------------------------------------------------------------
/**
 * Array to store get parameter name.
 */
//-------------------------------------------------------------------------------------------------
static char ParamsName[128] = "";
static bool ExitApp = true;
char TokenArray[MAX_NUMBER_OF_INPUT][MAX_LEN_OF_EACH_INPUT];

time_t StartTime, FinishTime;
time_t FirstWatchReportTime = -1;
uint32_t AcqRate;
uint32_t WatchPeriod;

//-------------------------------------------------------------------------------------------------
/**
 * Print the help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void PrintGnssHelp
(
    void
)
{
    puts("\n\t\tNAME:\n"
         "\t\t\tgnss - Used to access different functionality of gnss\n\n"
         "\t\tSYNOPSIS:\n"
         "\t\t\thelp\n"
         "\t\t\t<enable/disable>\n"
         "\t\t\t<start/stop>\n"
         "\t\t\trestart <RestartType>\n"
         "\t\t\tfix [FixTime in seconds]\n"
         "\t\t\tget <parameter>\n"
         "\t\t\tget posInfo\n"
         "\t\t\tset constellation <ConstellationType>\n"
         "\t\t\tset acqRate <acqRate in milliseconds>\n"
         "\t\t\tset nmeaSentences <nmeaMask>\n"
         "\t\t\tset nmeaConfig <nmeaMask> <datumType> <engineType>\n"
         "\t\t\tset minElevation <minElevation in degrees>\n"
         "\t\t\tset minGpsWeek <minGpsWeek in weeks>\n"
         "\t\t\tset startMode <StartMode>\n"
         "\t\t\t\t  be as follows:\n"
         "\t\t\t\t\t- 0 ---> HOT\n"
         "\t\t\t\t\t- 1 ---> WARM\n"
         "\t\t\t\t\t- 2 ---> COLD\n"
         "\t\t\t\t\t- 3 ---> FACTORY\n"
         "\t\t\t\t\t- 4 ---> UNKNOWN\n"
         "\t\t\tcapwatch [WatchPeriod in seconds]\n"
         "\t\t\tnmeawatch [WatchPeriod in seconds]\n"
         "\t\t\twatch [WatchPeriod in seconds]\n\n"
         "\t\tDESCRIPTION:\n"
         "\t\t\thelp\n"
         "\t\t\t\t- Print this help message and exit\n\n"
         "\t\t\t<enable/disable>\n"
         "\t\t\t\t- Enable/disable gnss device\n\n"
         "\t\t\t<start/stop>\n"
         "\t\t\t\t- Start/stop gnss device\n\n"
         "\t\t\tstartType <EngineType>\n"
         "\t\t\t\t- Set GNSS device with the specified engine type. Type can be as follows:\n"
         "\t\t\t\t\t- 0 ---> FUSED\n"
         "\t\t\t\t\t- 1 ---> SPE\n"
         "\t\t\t\t\t- 2 ---> PPE\n"
         "\t\t\t\t\t- 3 ---> VPE\n\n"
         "\t\t\tConfigLevArm <forwardOffset(double)> <sidewaysOffset(double)> <upOffset(double)> <levArmType>\n"
         "\t\t\t\t- Configure Lever Arm Parameters to support QDR.levArmType can be as follows:\n"
         "\t\t\t\t\t- GNSS_TO_VRP->1\n"
         "\t\t\t\t\t- DR_IMU_TO_GNSS->2\n"
         "\t\t\t\t\t- VPE_IMU_TO_GNSS->3\n\n"
         "\t\t\tConfigDR <rollOffset(double)> <yawOffset(double)> <pitchOffset(double)> <offsetUnc(double)>\n"
         "\t\t\t\t     <speedFactor(double)> <speedFactorUnc(double)> <gyroFactor(double)> <gyroFactorUnc(double>\n"
         "\t\t\t\t- Configure Dead Reckoning Engine Parameters to support QDR.\n\n"
         "\t\t\tDRValidity <Mask value>\n"
         "\t\t\t\t\t- 1->BODY_TO_SENSOR_MOUNT_PARAMS\n"
         "\t\t\t\t\t- 2->VEHICLE_SPEED_SCALE_FACTOR\n"
         "\t\t\t\t\t- 4->VEHICLE_SPEED_SCALE_FACTOR_UNC\n"
         "\t\t\t\t\t- 8->GYRO_SCALE_FACTOR\n"
         "\t\t\t\t\t- 16->GYRO_SCALE_FACTOR_UNC\n"
         "\t\t\tset configEng <EngineType> <EngineState>\n"
         "\t\t\t\t  Engine type be as follows:\n"
         "\t\t\t\t\t- 0 ---> UNKNOWN\n"
         "\t\t\t\t\t- 1 ---> SPE(Currently not supported)\n"
         "\t\t\t\t\t- 2 ---> PPE(Currently not supported)\n"
         "\t\t\t\t\t- 3 ---> DRE(supported only)\n"
         "\t\t\t\t\t- 4 ---> VPE(Currently not supported)\n"
         "\t\t\t\t  Engine state be as follows:\n"
         "\t\t\t\t\t- 0 ---> UNKNOWN\n"
         "\t\t\t\t\t- 1 ---> SUSPEND\n"
         "\t\t\t\t\t- 2 ---> RUNNING\n"
         "\t\t\tset robustloc <enable> <enabled911>\n"
         "\t\t\t\t- Configuring robust location information be as follows:\n"
         "\t\t\t\t\t-  enable->1 for enabling  enable->0 for disabling\n"
         "\t\t\t\t\t-  enable911->1 for enabling enable911->0 for disabling\n"
         "\t\t\tset secondBandConst <constellation type>"
         "\t\t\t\t  constellation type be as follows:\n"
         "\t\t\t\t\t- -1 ---> UNKNOWN\n"
         "\t\t\t\t\t-  1 ---> GPS\n"
         "\t\t\t\t\t-  2 ---> GALILEO\n"
         "\t\t\t\t\t-  4 ---> SBAS\n"
         "\t\t\t\t\t-  8 ---> COMPAS(Not supported)\n"
         "\t\t\t\t\t-  16 ---> GLONASS\n"
         "\t\t\t\t\t-  32 ---> BDS\n"
         "\t\t\t\t\t-  64 ---> QZSS\n"
         "\t\t\t\t\t-  128 ---> NAVIC\n"
         "\t\t\t<delete DRCalibData>\n"
         "\t\t\t\t- Deletes DR Sensor calibration data\n\n"
         "\t\t\trestart <RestartType>\n"
         "\t\t\t\t- Restart gnss device. Allowed when device in 'active' state. Restart type can\n"
         "\t\t\t\t  be as follows:\n"
         "\t\t\t\t\t- hot\n"
         "\t\t\t\t\t- warm\n"
         "\t\t\t\t\t- cold\n"
         "\t\t\t\t\t- factory\n"
         "\t\t\t\tTo know more about these restart types, please look at: \n"
         "\t\t\t\t           https://docs.legato.io/latest/c_gnss.html\n\n"
         "\t\t\tfix [FixTime in seconds]\n"
         "\t\t\t\t- Loop for certain time for first position fix. Here, FixTime is optional.\n"
         "\t\t\t\t  Default time(60s) will be used if not specified\n\n"
         "\t\t\tsupportedNmeaSentences --> Supported NMEA sentences (bit mask)\n\n"
         "\t\t\tsupportedConstellations --> Supported Constellations (bit mask)\n\n"
         "\t\t\tconfigDefSecBand --> Configure default secondary band constellations\n\n"
         "\t\t\tget <parameter>\n"
         "\t\t\t\t- Used to get different gnss parameter.\n"
         "\t\t\t\t  Follows parameters and their descriptions :\n"
         "\t\t\t\t\t- ttff          --> Time to First Fix (milliseconds)\n"
         "\t\t\t\t\t- acqRate       --> Acquisition Rate (unit milliseconds)\n"
         "\t\t\t\t\t- nmeaSentences --> Enabled NMEA sentences (bit mask)\n"
         "\t\t\t\t\t- minElevation  --> Minimum elevation in degrees\n"
         "\t\t\t\t\t- minGpsWeek    --> Minimum GPS week in weeks\n"
         "\t\t\t\t\t- locCap        --> Get location capabilities\n"
         "\t\t\t\t\t- constellation --> GNSS constellation\n"
         "\t\t\t\t\t- secondBandConst --> Secondary band Constellations\n"
         "\t\t\t\t\t- robustloc     --> Robust location information\n"
         "\t\t\t\t\t- magDev        --> Magnitude deviation\n"
         "\t\t\t\t\t- elliUnc       --> Elliptical Uncertainty\n"
         "\t\t\t\t\t- posState      --> Position fix state(no fix, 2D, 3D etc)\n"
         "\t\t\t\t\t- loc2d         --> 2D location (latitude, longitude, horizontal accuracy)\n"
         "\t\t\t\t\t- vrpLLA        --> VRP based latitude, longitude, altitude\n"
         "\t\t\t\t\t- vrpVel        --> VRP based east,north & up velocity information\n"
         "\t\t\t\t\t- svData        --> The satellite vehicles that are used to calculate position\n"
         "\t\t\t\t\t- sbasType      --> Navigation solution mask used to indicate SBAS corrections\n"
         "\t\t\t\t\t- techInfo      --> Position technology to indicate which technology is used.\n"
         "\t\t\t\t\t-                   The technology used in computing the fix\n"
         "\t\t\t\t\t- validityInfo  --> Validity of the Location basic Info.\n"
         "\t\t\t\t\t- engParams     --> The position engines & location engine type used in\n"
         "\t\t\t\t\t                    calculating the position report.\n"
         "\t\t\t\t\t- reliablityInfo -->Gets the reliability of the horizontal & vertical positions.\n"
         "\t\t\t\t\t- azimuthDevInfo -->Gets the elliptical horizontal uncertainty azimuth of\n"
         "\t\t\t\t\t-                   orientation,east and north standard deviations.\n"
         "\t\t\t\t\t- realTimeInfo  -->Gets the elapsed real time and its uncertainity in nano sec\n"
         "\t\t\t\t\t- measInfo      -->Gets the measurement usages information\n"
         "\t\t\t\t\t- reportStatus  -->Gets status of report in terms of how optimally the report was calculated by the engine\n"
         "\t\t\t\t\t- altMSeaLevel  -->Gets the altitude with respect to mean sea level in meters\n"
         "\t\t\t\t\t- svIds         -->Gets the GNSS Satellite Vehicles used in position data.\n"
         "\t\t\t\t\t- gnssData      -->Gets the GNSS data mask,Jammer and AGC data\n"
         "\t\t\t\t\t- gPTPTime      --> Get Gptp time stamp information\n"
         "\t\t\t\t\t- alt           --> Altitude (Altitude, Vertical accuracy)\n"
         "\t\t\t\t\t- loc3d         --> 3D location (latitude, longitude, altitude,\n"
         "\t\t\t\t\t                horizontal accuracy, vertical accuracy)\n"
         "\t\t\t\t\t- gpsTime       --> Get last updated gps time\n"
         "\t\t\t\t\t- time          --> Time of the last updated location\n"
         "\t\t\t\t\t- epochTime     --> Epoch time of the last updated location\n"
         "\t\t\t\t\t- timeAcc       --> Time accuracy in nanoseconds\n"
         "\t\t\t\t\t- LeapSeconds   --> Current and next leap seconds\n"
         "\t\t\t\t\t- LeapSecondsUnc --> Leap Seconds Uncertainty\n"
         "\t\t\t\t\t- GpsLeapSeconds --> UTC leap seconds in advance in seconds\n"
         "\t\t\t\t\t- date          --> Date of the last updated location\n"
         "\t\t\t\t\t- hSpeed        --> Horizontal speed(Horizontal Speed, Horizontal\n"
         "\t\t\t\t\t                    Speed accuracy)\n"
         "\t\t\t\t\t- vSpeed        --> Vertical speed(Vertical Speed, Vertical Speed accuracy)\n"
         "\t\t\t\t\t- motion        --> Motion data (Horizontal Speed, Horizontal Speed accuracy,\n"
         "\t\t\t\t\t                    Vertical Speed, Vertical Speed accuracy)\n"
         "\t\t\t\t\t- direction     --> Direction indication\n"
         "\t\t\t\t\t- satInfo       --> Satellites Vehicle information\n"
         "\t\t\t\t\t- satInfoEx     --> Extended Satellites Vehicle information\n"
         "\t\t\t\t\t- satStat       --> Satellites Vehicle status\n"
         "\t\t\t\t\t- dop           --> Dilution of Precision for the fixed position. Displayed\n"
         "\t\t\t\t\t-               in all resolutions: (0 to 3 digits after the decimal point) \n"
         "\t\t\t\t\t- posInfo       --> Get all current position info of the device\n"
         "\t\t\t\t\t- conformIndex  --> Get the comformity index for robust location\n"
         "\t\t\t\t\t- 0.0 ---> Least comforming\n"
         "\t\t\t\t\t- 1.0 ---> Most comforming\n"
         "\t\t\t\t\t- calibData  --> Get the sensor calibration status and confidence percent\n"
         "\t\t\t\t\t- drSolutionStatus  --> Get the the Dead Reckoning sensor solution status\n"
         "\t\t\t\t\t- bodyFrameData --> Get Kinematics information related to body parameters\n"
         "\t\t\t\t\t- xtraStatus --> Get the device's xtra status\n"
         "\t\t\t\t\t- status        --> Get gnss device's current status\n\n"
         "\t\t\tset constellation <ConstellationType>\n"
         "\t\t\t\t- Used to set constellation. Allowed when device in 'ready/Active' state. May require\n"
         "\t\t\t\t  platform reboot, please look platform documentation for details.\n"
         "\t\t\t\t  ConstellationType can be as follows:\n"
         "\t\t\t\t\t- 1 ---> DEFAULT\n"
         "\t\t\t\t\t- 2 ---> GPS\n"
         "\t\t\t\t\t- 4 ---> GLONASS\n"
         "\t\t\t\t\t- 8 ---> BEIDOU\n"
         "\t\t\t\t\t- 16 ---> GALILEO\n"
         "\t\t\t\t\t- 32 --> SBAS\n"
         "\t\t\t\t\t- 64 --> QZSS\n"
         "\t\t\t\t\t- 128 --> NAVIC\n"
         "\t\t\t\tPlease use sum of the values to set multiple constellation, e.g.\n"
         "\t\t\t\t10 for GLONASS+GALILEO, 46 for GLONASS+BEIDOU+GALILEO+QZSS\n\n"
         "\t\t\tset acqRate <acqRate in milliseconds>\n"
         "\t\t\t\t- Used to set acquisition rate.\n"
         "\t\t\t\t  Please note that it is available when the device is 'ready' state.\n\n"
         "\t\t\tset nmeaSentences <nmeaMask>\n"
         "\t\t\tset nmeaConfig <nmeaMask> <datumType> <engineType>\n"
         "\t\t\t\t- Used to set the enabled NMEA sentences. \n"
         "\t\t\t\t  Bit mask should be set with hexadecimal values, e.g. 7FFF\n\n"
         "\t\t\t\t- Used to set nmea sentences. Allowed when device in 'ready' state. May require\n"
         "\t\t\t\t  platform reboot, please look platform documentation for details.\n"
         "\t\t\t\t  nmeaMask can be as follows (the values are in hexadecimal):\n"
         "\t\t\t\t\t- 1 ------> GPGGA\n"
         "\t\t\t\t\t- 2 ------> GARMC\n"
         "\t\t\t\t\t- 4 ------> GNGSA\n"
         "\t\t\t\t\t- 8 ------> GPVTG\n"
         "\t\t\t\t\t- 10 -----> GPGNS\n"
         "\t\t\t\t\t- 20 -----> GPDTM\n"
         "\t\t\t\t\t- 40 -----> GPGSV\n"
         "\t\t\t\t\t- 80 -----> GLGSV\n"
         "\t\t\t\t\t- 100 ----> GAGSV\n"
         "\t\t\t\t\t- 200 ----> GQGSV\n"
         "\t\t\t\t\t- 400 ----> GBGSV\n"
         "\t\t\t\t\t- 800 ----> GIGSV\n"
         "\t\t\t\t\t- FFFFFFFF ---> ALL\n"
         "\t\t\t\t  datumType can be as follows:\n"
         "\t\t\t\t- 0-GEODETIC_TYPE_WGS_84\n"
         "\t\t\t\t- 1-GEODETIC_TYPE_PZ_90\n"
         "\t\t\t\t  engineType can be as follows:\n"
         "\t\t\t\t- 1-LOC_ENGINE_FUSED\n"
         "\t\t\t\t- 2-LOC_ENGINE_SPE\n"
         "\t\t\t\t- 3-LOC_ENGINE_PPE\n"
         "\t\t\t\t- 4-LOC_ENGINE_VPE\n"
         "\t\t\tset minElevation <minElevation in degrees>\n"
         "\t\t\t\t- Used to set the minimum elevation in degrees [range 0..90].\n\n"
         "\t\t\tset minGpsWeek <minGpsWeek value>\n"
         "\t\t\twatch [WatchPeriod in seconds]\n"
         "\t\t\t\t- Used to monitor all gnss information(position, speed, satellites used etc).\n"
         "\t\t\t\t  Here, WatchPeriod is optional. Default time(600s) will be used if not\n"
         "\t\t\t\t  specified\n\n"
         "\tPlease note, some commands require gnss device to be in specific state\n"
         "\t(and platform reboot) to produce valid result. Please look :\n"
         "\thttps://docs.legato.io/latest/howToGNSS.html,\n"
         "\thttps://docs.legato.io/latest/c_gnss.html and platform documentation for more\n"
         "\tdetails.\n\n"
         "\t? / h - For help\n"
         "\tq / 0 - To exit\n"
         );
}


//-------------------------------------------------------------------------------------------------
/**
 * This function enables gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int Enable
(
    void
)
{
    le_result_t result = LE_FAULT;

    result = le_gnss_Enable();

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_DUPLICATE:
            printf("The GNSS device is already enabled\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not initialized\n");
            break;
        case LE_FAULT:
            printf("Failed to enable GNSS device\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}



//-------------------------------------------------------------------------------------------------
/**
 * This function disables gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int Disable
(
    void
)
{
    le_result_t result = LE_FAULT;

    result = le_gnss_Disable();

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_DUPLICATE:
            printf("The GNSS device is already disabled\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not initialized or started. Please see log for details\n");
            break;
        case LE_FAULT:
            printf("Failed to disable GNSS device\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function starts gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int Start
(
    void
)
{
    le_result_t result = LE_FAULT;

    result = le_gnss_Start();

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_DUPLICATE:
            printf("The GNSS device is already started\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is disabled or not initialized. See logs for details\n");
            break;
        case LE_FAULT:
            printf("Failed to start GNSS device. See logs for details\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/**
 * This function starts the GNSS device in the specified engine type.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int StartEngineType
(
    const char* startTypePtr           ///< [IN] Start type
)
{
    char *end;
    uint32_t type = strtoul(startTypePtr, &end, BASE10);

    if ('\0' != end[0])
    {
        printf("Bad type : %s\n", startTypePtr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_SetEngineType(type);

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_FAULT:
            printf("Failed to start the specified Engine type\n");
            break;
        case LE_BAD_PARAMETER:
            printf("Bad parameter to set SetEngineType\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not ready state\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/**
 * This function configures Lever Arm Parameters.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int ConfigureLevArm
(
    taf_locGnss_LeverArmParams_t* LeverArmParams         ///< [IN] Lever Arm Parameters
)
{

    le_result_t result = le_gnss_SetLeverArmConfig(LeverArmParams);

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_FAULT:
            printf("Failed to configure Lever Arm parameters\n");
            break;
        case LE_BAD_PARAMETER:
            printf("Bad parameter to configure Lever Arm parameters\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in Ready State\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }
    //free the LeverArmParams memory
    le_mem_Release(LeverArmParams);
    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/**
 * This function configures Dead Reckoning Engine Parameters.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int ConfigureDeadReckoning
(
    taf_locGnss_DrParams_t* DrParams         ///< [IN] Lever Arm Parameters
)
{

    le_result_t result = le_gnss_SetDRConfig(DrParams);

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_FAULT:
            printf("Failed to configure Dead Reckoning Engine parameters\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in Ready State\n");
            break;
        case LE_OUT_OF_RANGE:
            printf("Dead reckoning parameters out of range\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }
    //free the DrParams memory
    le_mem_Release(DrParams);
    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function stops gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int Stop
(
    void
)
{
    le_result_t result = LE_FAULT;

    result = le_gnss_Stop();

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_DUPLICATE:
            printf("The GNSS device is already stopped\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not initialized or disabled. See logs for details\n");
            break;
        case LE_FAULT:
            printf("Failed to stop GNSS device. See logs for details\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function restarts gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int Restart
(
    const char* restartTypePtr      ///< [IN] Type of restart, i.e. hot/warm/cold/factory etc
)
{
    le_result_t result = LE_FAULT;

    if (strcmp(restartTypePtr, "cold") == 0)
    {
        printf("Doing cold restart...\n");
        result = le_gnss_ForceColdRestart();
    }
    else if (strcmp(restartTypePtr, "warm") == 0)
    {
        printf("Doing warm restart...\n");
        result = le_gnss_ForceWarmRestart();
    }
    else if (strcmp(restartTypePtr, "hot") == 0)
    {
        printf("Doing hot restart...\n");
        result = le_gnss_ForceHotRestart();
    }
    else if (strcmp(restartTypePtr, "factory") == 0)
    {
        printf("Doing factory restart...\n");
        result = le_gnss_ForceFactoryRestart();
    }
    else
    {
        printf("Invalid parameter: %s\n", restartTypePtr);
        return EXIT_FAILURE;
    }

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not enabled or not started. See logs for details\n");
            break;
        case LE_FAULT:
            printf("Failed to do '%s' restart. See logs for details\n", restartTypePtr);
            break;
        case LE_UNSUPPORTED:
            printf("Not supported\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function deletes DR sensor calibration data.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int Delete
(
    const char* deleteTypePtr      ///< [IN] Type of delete, i.e. DR sensor calib data
)
{
    le_result_t result = LE_FAULT;
    if (strcmp(deleteTypePtr, "DRCalibData") == 0)
    {
        printf("Doing DR sensor calib data deletion..\n");
        result = le_gnss_DeleteDRSensorCalData();
    }
    else
    {
        printf("Invalid parameter: %s\n", deleteTypePtr);
        return EXIT_FAILURE;
    }

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not READY. See logs for details\n");
            break;
        case LE_FAULT:
            printf("Failed to delete DR sensor calibration data\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function sets gnss device acquisition rate.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int SetAcquisitionRate
(
    const char* acqRateStr          ///< [IN] Acquisition rate in milliseconds
)
{
    char *end;
    uint32_t acqRate = strtoul(acqRateStr, &end, BASE10);

    if ('\0' != end[0])
    {
        printf("Bad acquisition rate: %s\n", acqRateStr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_SetAcquisitionRate(acqRate);

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_FAULT:
            printf("Failed to Set acquisition rate\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in \"Ready \" state\n");
            break;
        case LE_OUT_OF_RANGE:
            printf("Out of range! Acq rate is less than 100ms \n");
            break;
        case LE_TIMEOUT:
            printf("Timeout error\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function sets the GNSS minimum elevation.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int SetMinElevation
(
    const char* minElevationPtr           ///< [IN] Minimum elevation in degrees [range 0..90]
)
{
    char *end;
    uint32_t minElevation = strtoul(minElevationPtr, &end, BASE10);

    if ('\0' != end[0])
    {
        printf("Bad minimum elevation: %s\n", minElevationPtr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_SetMinElevation(minElevation);

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_FAULT:
            printf("Failed to set the minimum elevation\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in \"Ready\" state\n");
            break;
        case LE_OUT_OF_RANGE:
            printf("The minimum elevation is above range\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function sets the minimum GPS week.
 *
 * @return
 *  - LE_OK             Succeeded.
 *  - LE_FAULT          Failed.
 *  - LE_NO_MEMORY      The minGpsWeekPtr is NULL.
 *  - LE_NOT_PERMITTED  The GNSS device state is not ready.
 */
//-------------------------------------------------------------------------------------------------
static int SetMinGpsWeek
(
    const char* minGpsWeekPtr           ///< [IN] Minimum GPS week
)
{
    char *end;
    uint16_t minGpsWeek = strtoul(minGpsWeekPtr, &end, BASE10);

    if ('\0' != end[0])
    {
        printf("Bad minimum GPS week: %s\n", minGpsWeekPtr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_SetMinGpsWeek(minGpsWeek);

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_FAULT:
            printf("Failed to set the minimum GPS week\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in \"Ready\" state\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the minimum GPS week.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetMinGpsWeek
(
    void
)
{
    uint16_t  minGpsWeek;
    le_result_t result = le_gnss_GetMinGpsWeek(&minGpsWeek);

    switch (result)
    {
        case LE_OK:
            printf("Minimum GPS week: %d\n", minGpsWeek);
            break;
        case LE_FAULT:
            printf("Failed to get the minimum GPS week. See logs for details\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in \"Ready\" state\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

void DisplayCapabilities(taf_locGnss_LocCapabilityType_t capabilityMask) {
  printf("\n************* Capabilities Information *************\n");
  printf("The location capabilities bit mask: %xlu\n", capabilityMask);
  if (capabilityMask & TAF_LOCGNSS_TIME_BASED_TRACKING) {
    printf("Time based tracking\n");
  }
  if (capabilityMask & TAF_LOCGNSS_DISTANCE_BASED_TRACKING) {
    printf("Distance based tracking\n");
  }
  if (capabilityMask & TAF_LOCGNSS_GNSS_MEASUREMENTS) {
    printf("GNSS Measurement\n");
  }
  if (capabilityMask & TAF_LOCGNSS_CONSTELLATION_ENABLEMENT) {
    printf("Constellation enablement\n");
  }
  if (capabilityMask & TAF_LOCGNSS_CARRIER_PHASE) {
    printf("Carrier phase\n");
  }
  if (capabilityMask & TAF_LOCGNSS_QWES_GNSS_SINGLE_FREQUENCY) {
    printf("QWES GNSS single frequency\n");
  }
  if (capabilityMask & TAF_LOCGNSS_QWES_GNSS_MULTI_FREQUENCY) {
    printf("QWES GNSS multi frequency\n");
  }
  if (capabilityMask & TAF_LOCGNSS_QWES_VPE) {
    printf("QWES VPE\n");
  }
  if (capabilityMask & TAF_LOCGNSS_QWES_CV2X_LOCATION_BASIC) {
    printf("QWES CV2X location basic\n");
  }
  if (capabilityMask & TAF_LOCGNSS_QWES_CV2X_LOCATION_PREMIUM) {
    printf("QWES CV2X location premium\n");
  }
  if (capabilityMask & TAF_LOCGNSS_QWES_PPE) {
    printf("QWES PPE\n");
  }
  if (capabilityMask & TAF_LOCGNSS_QWES_QDR2) {
    printf("QWES QDR2\n");
  }
  if (capabilityMask & TAF_LOCGNSS_QWES_QDR3) {
    printf("QWES QDR3\n");
  }
  if (capabilityMask & TAF_LOCGNSS_TIME_BASED_BATCHING) {
    printf("TIME_BASED_BATCHING\n");
  }
  if (capabilityMask & TAF_LOCGNSS_DISTANCE_BASED_BATCHING) {
    printf("DISTANCE_BASED_BATCHING\n");
  }
  if (capabilityMask & TAF_LOCGNSS_GEOFENCE) {
    printf("GEOFENCE\n");
  }
  if (capabilityMask & TAF_LOCGNSS_OUTDOOR_TRIP_BATCHING) {
    printf("OUTDOOR_TRIP_BATCHING\n");
  }
  if (capabilityMask & TAF_LOCGNSS_SV_POLYNOMIAL) {
    printf("SV_POLYNOMIAL\n");
  }
  if (capabilityMask & TAF_LOCGNSS_NLOS_ML20) {
    printf("NLOS_ML20\n");
  }
  printf("****************************************************\n");
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the location capabilities.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetCapabilities
(
    void
)
{
    uint64_t  locCapability;
    le_result_t result = le_gnss_GetCapabilities(&locCapability);

    switch (result)
    {
        case LE_OK:
            DisplayCapabilities(locCapability);
            break;
        case LE_FAULT:
            printf("Failed to get the location capabilities. See logs for details\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function configure the NMEA sentences.
 *
 * @return
 *  - LE_OK             Succeeded.
 *  - LE_BAD_PARAMETER  the nmeaMask is zero.
 *  - LE_FAULT          Failed.
 *  - LE_NOT_PERMITTED  GNSS device is not ready.
 *
 */
//-------------------------------------------------------------------------------------------------
static int SetNmeaConfiguration
(
    const char* nmeaMaskPtr,           ///< [IN] Enabled NMEA sentences bit mask
    const char* datumTypePtr,          ///< [IN] Specify the datum type to be configured.
    const char* engineTypePtr          ///< [IN] Specify the engine type to be configured.
)
{
    uint64_t nmeaMask = strtoull(nmeaMaskPtr, NULL, 16);
    int datumType = le_hex_HexaToInteger(datumTypePtr);
    int engineType = le_hex_HexaToInteger(engineTypePtr);

    LE_INFO("SetNmeaConfiguration nmeaMask: %" PRIu64 ", datumType: %d, engineType: %d",nmeaMask, datumType, engineType);

    le_result_t result = le_gnss_SetNmeaConfiguration(nmeaMask, (le_gnss_GeodeticDatumType_t) datumType, engineType);

    switch (result)
    {
        case LE_OK:
            printf("Successfully enabled the NMEA!\n");
            break;
        case LE_FAULT:
            printf("Failed to set NMEA. See logs for details\n");
            break;
        case LE_BAD_PARAMETER:
            printf("Failed to set NMEA, incompatible bit mask\n");
            break;
       case LE_NOT_PERMITTED:
            printf("SetNmeaConfiguration: GNSS is not in ready state!\n");
            break;
        default:
            printf("Failed to set NMEA, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the xtra status.
 *
 * @return
 *  - LE_OK             Succeeded.
 *  - LE_FAULT          Failed.
 *  - LE_NOT_PERMITTED  GNSS device is not ready.
 *
 */
//-------------------------------------------------------------------------------------------------
static int GetXtraStatus
(
    void
)
{
    taf_locGnss_XtraStatusParams_t *XtraParamsPtr;
    le_result_t result = LE_FAULT;
    le_mem_PoolRef_t XtraFramePool = NULL;
    XtraFramePool = le_mem_CreatePool("XtraFramePool", sizeof(taf_locGnss_XtraStatusParams_t));
    XtraParamsPtr = (taf_locGnss_XtraStatusParams_t*) le_mem_ForceAlloc(XtraFramePool);
    if(XtraParamsPtr != NULL)
    {
        result = le_gnss_GetXtraStatus(XtraParamsPtr);
    }
    else
    {
        printf("XtraParamsPtr is NULL pointer\n");
    }

    switch (result)
    {
        case LE_OK:
            printf("**** Request Xtra Status Info ****\n");
            printf("GetXtraStatus featureEnabled:%d\n",XtraParamsPtr->featureEnabled);
            printf("GetXtraStatus xtraDataStatus:");
            switch(XtraParamsPtr->xtraDataStatus)
            {
                case LE_GNSS_XTRA_DATA_STATUS_UNKNOWN:
                    printf("Unknown\n");
                    break;
                case LE_GNSS_XTRA_DATA_STATUS_NOT_AVAIL:
                    printf("Not available\n");
                    break;
                case LE_GNSS_XTRA_DATA_STATUS_NOT_VALID:
                    printf("Invalid\n");
                    break;
                case LE_GNSS_XTRA_DATA_STATUS_VALID:
                    printf("Valid\n");
                    break;
                default:
                    printf("No xtra status\n");
                break;
            }
            printf("GetXtraStatus xtraValidForHours:%d\n",XtraParamsPtr->xtraValidForHours);
            break;
        case LE_FAULT:
            printf("Failed to get xtra status. See logs for details\n");
            break;
       case LE_NOT_PERMITTED:
            printf("Device is not in ready/active state!\n");
            break;
        default:
            printf("Failed to get xtra status, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }

    //release the memory
    le_mem_Release(XtraParamsPtr);

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function Gets Gptp time and its uncertainity.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetGptpTimeInformation
(
    le_gnss_SampleRef_t positionSampleRef   ///< [IN] Position sample reference
)
{
    uint64_t gPtpTime;
    uint64_t gPtpTimeUnc;

    le_result_t result = le_gnss_GetGptpTime(positionSampleRef,&gPtpTime,&gPtpTimeUnc);

    switch (result)
    {
        case LE_OK:
            printf("Gptp Time(in ns) :%"PRIu64"\n",gPtpTime);
            printf("Gptp Time Uncertainity(in ns) :%"PRIu64"\n",gPtpTimeUnc);
            break;
        case LE_FAULT:
            printf("Failed to get gptp time Information\n");
            break;;
        default:
            printf("Failed to get gptp time Information, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function starts the GNSS device in the specified start mode.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int StartMode
(
    const char* startModePtr           ///< [IN] Start mode
)
{
    char *end;
    uint32_t mode = strtoul(startModePtr, &end, BASE10);

    if ('\0' != end[0])
    {
        printf("Bad mode : %s\n", startModePtr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_StartMode(mode);

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_FAULT:
            printf("Failed to set the specified start Mode\n");
            break;
        case LE_DUPLICATE:
            printf("The GNSS device is already in Active State\n");
            break;
        case LE_BAD_PARAMETER:
            printf("Bad parameter to set StartMode\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in Ready State\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function configure Engine state for the specified engine type.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int ConfigureEngineState
(
    const char* engineTypePtr,           ///< [IN] Engine type
    const char* engineStatePtr           ///< [IN] Engine state
)
{
    char *end;
    uint32_t engineType = strtoul(engineTypePtr, &end, BASE10);

    if ('\0' != end[0])
    {
        printf("Bad engine type : %s\n", engineTypePtr);
        return EXIT_FAILURE;
    }

    uint32_t engineState = strtoul(engineStatePtr, &end, BASE10);
    if ('\0' != end[0])
    {
        printf("Bad engine state : %s\n", engineStatePtr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_ConfigureEngineState(engineType,engineState);

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_FAULT:
            printf("Failed to set engine state for the specified engine type\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in Ready/Active State\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function configure Robust Location finromation.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int ConfigureRobustLocation
(
    const char* enablePtr,           ///< [IN] Enable
    const char* enable911Ptr         ///< [IN] Enable911
)
{
    char *end;
    uint32_t enable = strtoul(enablePtr, &end, BASE10);

    if ('\0' != end[0])
    {
        printf("Bad enable : %s\n", enablePtr);
        return EXIT_FAILURE;
    }

    uint32_t enable911 = strtoul(enable911Ptr, &end, BASE10);
    if ('\0' != end[0])
    {
        printf("Bad enable911 : %s\n", enable911Ptr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_ConfigureRobustLocation(enable,enable911);

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_FAULT:
            printf("Failed to configure Robust location information\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in Ready/Active State\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function configure secondary band constellations.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int ConfigureSecondaryBandConstellations
(
    const char* secondBandConstPtr        ///< [IN] secondary band constellation
)
{
    uint32_t constellationMask = 0;

    char *endPtr;
    errno = 0;
    int constellationSum = strtoul(secondBandConstPtr, &endPtr, 10);

    if (endPtr[0] != '\0' || errno != 0 || constellationSum == 0)
    {
        fprintf(stderr, "Bad constellation parameter: %s\n", secondBandConstPtr);
        return EXIT_FAILURE;
    }

    char constellationStr[CONSTELLATIONS_NAME_LEN] = "[";

    if (constellationSum & (1<<(LE_GNSS_SB_CONSTELLATION_GPS-1)))
    {
        constellationMask |= (uint32_t)(1<<(LE_GNSS_SB_CONSTELLATION_GPS-1));
        constellationSum -= (1<<(LE_GNSS_SB_CONSTELLATION_GPS-1));
        le_utf8_Append(constellationStr, "GPS ", sizeof(constellationStr), NULL);
    }
    if (constellationSum & (1<<(LE_GNSS_SB_CONSTELLATION_GALILEO-1)))
    {
        constellationMask |= (uint32_t)(1<<(LE_GNSS_SB_CONSTELLATION_GALILEO-1));
        constellationSum -= (1<<(LE_GNSS_SB_CONSTELLATION_GALILEO-1));
        le_utf8_Append(constellationStr, "GALILEO ", sizeof(constellationStr), NULL);
    }
    if (constellationSum & (1<<(LE_GNSS_SB_CONSTELLATION_SBAS-1)))
    {
        constellationMask |= (uint32_t)(1<<(LE_GNSS_SB_CONSTELLATION_SBAS-1));
        constellationSum -= (1<<(LE_GNSS_SB_CONSTELLATION_SBAS-1));
        le_utf8_Append(constellationStr, "SBAS ", sizeof(constellationStr), NULL);
    }
    if (constellationSum & (1<<(LE_GNSS_SB_CONSTELLATION_GLONASS-1)))
    {
        constellationMask |= (uint32_t)(1<<(LE_GNSS_SB_CONSTELLATION_GLONASS-1));
        constellationSum -= (1<<(LE_GNSS_SB_CONSTELLATION_GLONASS-1));
        le_utf8_Append(constellationStr, "GLONASS ", sizeof(constellationStr), NULL);
    }
    if (constellationSum & (1<<(LE_GNSS_SB_CONSTELLATION_BDS-1)))
    {
        constellationMask |= (uint32_t)(1<<(LE_GNSS_SB_CONSTELLATION_BDS-1));
        constellationSum -= (1<<(LE_GNSS_SB_CONSTELLATION_BDS-1));
        le_utf8_Append(constellationStr, "BDS ", sizeof(constellationStr), NULL);
    }
    if (constellationSum & (1<<(LE_GNSS_SB_CONSTELLATION_QZSS-1)))
    {
        constellationMask |= (uint32_t)(1<<(LE_GNSS_SB_CONSTELLATION_QZSS-1));
        constellationSum -= (1<<(LE_GNSS_SB_CONSTELLATION_QZSS-1));
        le_utf8_Append(constellationStr, "QZSS ", sizeof(constellationStr), NULL);
    }
    if (constellationSum & (1<<(LE_GNSS_SB_CONSTELLATION_NAVIC-1)))
    {
        constellationMask |= (uint32_t)(1<<(LE_GNSS_SB_CONSTELLATION_NAVIC-1));
        constellationSum -= (1<<(LE_GNSS_SB_CONSTELLATION_NAVIC-1));
        le_utf8_Append(constellationStr, "NAVIC ", sizeof(constellationStr), NULL);
    }
    le_utf8_Append(constellationStr, "]", sizeof(constellationStr), NULL);

    LE_INFO("Configuring secondary constellation %s",constellationStr);

    // Right now all constellation sum should be zero
    if (constellationSum != 0)
    {
        fprintf(stderr, "Bad constellation parameter: %s\n", secondBandConstPtr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_ConfigureSecondaryBandConstellations(constellationMask);

    switch (result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_FAULT:
            printf("Failed to configure secondary band constellations\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in Ready State\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function sets constellation of gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int SetConstellation
(
    const char* constellationPtr   ///< [IN] GNSS constellation used in solution
)
{
    uint32_t constellationMask = 0;

    char *endPtr;
    errno = 0;
    int constellationSum = strtoul(constellationPtr, &endPtr, 10);

    if (endPtr[0] != '\0' || errno != 0 || constellationSum == 0)
    {
        fprintf(stderr, "Bad constellation parameter: %s\n", constellationPtr);
        return EXIT_FAILURE;
    }

    char constellationStr[CONSTELLATIONS_NAME_LEN] = "[";
    if (constellationSum & CONSTELLATION_DEFAULT)
    {
        constellationMask |= (uint32_t)LE_GNSS_CONSTELLATION_DEFAULT;
        constellationSum -= CONSTELLATION_DEFAULT;
        LE_INFO("constellation type is CONSTELLATION_DEFAULT");
        le_utf8_Append(constellationStr, "DEFAULT ", sizeof(constellationStr), NULL);
    }
    if (constellationSum & CONSTELLATION_GPS)
    {
        constellationMask |= (uint32_t)LE_GNSS_CONSTELLATION_GPS;
        constellationSum -= CONSTELLATION_GPS;
        le_utf8_Append(constellationStr, "GPS ", sizeof(constellationStr), NULL);
    }
    if (constellationSum & CONSTELLATION_GLONASS)
    {
        constellationMask |= (uint32_t)LE_GNSS_CONSTELLATION_GLONASS;
        constellationSum -= CONSTELLATION_GLONASS;
        le_utf8_Append(constellationStr, "GLONASS ", sizeof(constellationStr), NULL);
    }
    if (constellationSum & CONSTELLATION_BEIDOU)
    {
        constellationMask |= (uint32_t)LE_GNSS_CONSTELLATION_BEIDOU;
        constellationSum -= CONSTELLATION_BEIDOU;
        le_utf8_Append(constellationStr, "BEIDOU ", sizeof(constellationStr), NULL);
    }
    if (constellationSum & CONSTELLATION_GALILEO)
    {
        constellationMask |= (uint32_t)LE_GNSS_CONSTELLATION_GALILEO;
        constellationSum -= CONSTELLATION_GALILEO;
        le_utf8_Append(constellationStr, "GALILEO ", sizeof(constellationStr), NULL);
    }
    if (constellationSum & CONSTELLATION_SBAS)
    {
        constellationMask |= (uint32_t)LE_GNSS_CONSTELLATION_SBAS;
        constellationSum -= CONSTELLATION_SBAS;
        le_utf8_Append(constellationStr, "SBAS ", sizeof(constellationStr), NULL);
    }
    if (constellationSum & CONSTELLATION_QZSS)
    {
        constellationMask |= (uint32_t)LE_GNSS_CONSTELLATION_QZSS;
        constellationSum -= CONSTELLATION_QZSS;
        le_utf8_Append(constellationStr, "QZSS ", sizeof(constellationStr), NULL);
    }
    if (constellationSum & CONSTELLATION_NAVIC)
    {
        constellationMask |= (uint32_t)LE_GNSS_CONSTELLATION_NAVIC;
        constellationSum -= CONSTELLATION_NAVIC;
        le_utf8_Append(constellationStr, "NAVIC ", sizeof(constellationStr), NULL);
    }
    le_utf8_Append(constellationStr, "]", sizeof(constellationStr), NULL);

    LE_INFO("Setting constellation %s",constellationStr);

    // Right now all constellation sum should be zero
    if (constellationSum != 0)
    {
        fprintf(stderr, "Bad constellation parameter: %s\n", constellationPtr);
        return EXIT_FAILURE;
    }

    le_result_t result =
       le_gnss_SetConstellation((le_gnss_ConstellationBitMask_t)constellationMask);

    switch(result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not initialized, disabled. See logs for  \
                    details\n");
            break;
        case LE_FAULT:
            printf("Failed!\n");
            break;
        default:
            printf("Bad return value: %d\n", result);
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#if 0
//-------------------------------------------------------------------------------------------------
/**
 * This function sets the area for a given constellation
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int SetConstellationArea
(
    const char* constellationPtr,      ///< [IN] GNSS constellation used in solution
    const char* constellationAreaPtr   ///< [IN] GNSS constellation area
)
{
    char *endPtr;
    errno = 0;
    int constellation = strtoul(constellationPtr, &endPtr, BASE10);
    int constArea = strtoul(constellationAreaPtr, &endPtr, BASE10);

    if (('\0' != endPtr[0]) || (0 != errno) || (0 == constellation) || (0 == constArea))
    {
        fprintf(stderr, "Bad constellation or area parameter: %s %s\n", constellationPtr,
                                                                        constellationAreaPtr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_SetConstellationArea((le_gnss_Constellation_t)constellation,
                                                      (le_gnss_ConstellationArea_t) constArea);
    switch(result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not initialized, disabled or active. See logs for  \
                    details\n");
            break;
        case LE_FAULT:
            printf("Failed!\n");
            break;
        case LE_BAD_PARAMETER:
            printf("Invalid area\n");
            break;
        default:
            printf("Bad return value: %d\n", result);
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif

#if 0
//-------------------------------------------------------------------------------------------------
/**
 * This function sets agps mode of gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int SetAgpsMode
(
    const char* agpsModePtr    ///< [IN] agps to set
)
{
    le_gnss_AssistedMode_t suplAgpsMode;

    if (strcmp(agpsModePtr, "alone") == 0)
    {
        suplAgpsMode = LE_GNSS_STANDALONE_MODE;
    }
    else if (strcmp(agpsModePtr,"msBase") == 0)
    {
        suplAgpsMode = LE_GNSS_MS_BASED_MODE;
    }
    else if (strcmp(agpsModePtr,"msAssist") == 0)
    {
        suplAgpsMode = LE_GNSS_MS_ASSISTED_MODE;
    }
    else
    {
        printf("Bad agps mode: %s\n", agpsModePtr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_SetSuplAssistedMode(suplAgpsMode);

    switch(result)
    {
        case LE_OK:
            printf("Success!\n");
            break;
        case LE_TIMEOUT:
            printf("Timeout error\n");
            break;
        case LE_FAULT:
            printf("Failed!\n");
            break;
        default:
            printf("Bad return value: %d\n", result);
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#endif
//-------------------------------------------------------------------------------------------------
/**
 * This function sets the enabled NMEA sentences.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int SetNmeaSentences
(
    const char* nmeaMaskStr     ///< [IN] Enabled NMEA sentences bit mask
)
{
    uint64_t nmeaMask = strtoull(nmeaMaskStr, NULL, 16);
    LE_INFO("SetNmeaSentences nmeaMask: %" PRIu64"", nmeaMask);

    le_result_t result = le_gnss_SetNmeaSentences(nmeaMask);

    switch (result)
    {
        case LE_OK:
            printf("Successfully set enabled NMEA sentences!\n");
            break;
        case LE_FAULT:
            printf("Failed to set enabled NMEA sentences. See logs for details\n");
            break;
        case LE_BAD_PARAMETER:
            printf("Failed to set enabled NMEA sentences, incompatible bit mask\n");
            break;
       case LE_NOT_PERMITTED:
            printf("GNSS is neither ready nor active state!\n");
            break;
        default:
            printf("Failed to set enabled NMEA sentences, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets TTFF (Time to First Fix) value.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetTtff
(
    le_gnss_State_t state    ///< [IN] GNSS state
)
{
    uint32_t ttff;
    le_result_t result;

    if ((LE_GNSS_STATE_ACTIVE != state) && (LE_GNSS_STATE_READY != state))
    {
        printf("GNSS is not in active or ready state!\n");
        return EXIT_FAILURE;
    }

    result = le_gnss_GetTtff(&ttff);
    switch (result)
    {
        case LE_OK:
            printf("TTFF(Time to First Fix)   : %ums\n", ttff);
            break;
        case LE_BUSY:
            printf("TTFF not calculated (Position not fixed)\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not started or disabled. See logs for details\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}
#if 0
//-------------------------------------------------------------------------------------------------
/**
 * This function gets the agps mode.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetAgpsMode
(
    void
)
{
    le_gnss_AssistedMode_t assistedMode;
    le_result_t result = le_gnss_GetSuplAssistedMode(&assistedMode);

    if (result == LE_OK)
    {
        switch (assistedMode)
        {
            case LE_GNSS_STANDALONE_MODE:
                printf("AGPS mode: Standalone\n");
                break;
            case LE_GNSS_MS_BASED_MODE:
                printf("AGPS mode: MS-based\n");
                break;
            case LE_GNSS_MS_ASSISTED_MODE:
                printf("AGPS mode: MS-assisted\n");
                break;
        }
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif

//-------------------------------------------------------------------------------------------------
/**
 * This function gets constellation of gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetConstellation
(
    void
)
{
    le_gnss_ConstellationBitMask_t constellationMask;
    le_result_t result = le_gnss_GetConstellation(&constellationMask);

    if (result == LE_OK)
    {
        printf("ConstellationType %d\n", constellationMask);

        (constellationMask & LE_GNSS_CONSTELLATION_GPS)     ? printf("***GPS blacklisted***\n") :
                                                              printf("GPS not blacklisted\n");
        (constellationMask & LE_GNSS_CONSTELLATION_GLONASS) ? printf("***GLONASS blacklisted***\n") :
                                                              printf("GLONASS not blacklisted\n");
        (constellationMask & LE_GNSS_CONSTELLATION_BEIDOU)  ? printf("***BEIDOU blacklisted***\n") :
                                                              printf("BEIDOU not blacklisted\n");
        (constellationMask & LE_GNSS_CONSTELLATION_GALILEO) ? printf("***GALILEO blacklisted***\n") :
                                                              printf("GALILEO not blacklisted\n");
        (constellationMask & LE_GNSS_CONSTELLATION_SBAS)    ? printf("***SBAS blacklisted***\n") :
                                                              printf("SBAS not blacklisted\n");
        (constellationMask & LE_GNSS_CONSTELLATION_QZSS)    ? printf("***QZSS blacklisted***\n") :
                                                              printf("QZSS not blacklisted\n");
        (constellationMask & LE_GNSS_CONSTELLATION_NAVIC)   ? printf("***NAVIC blacklisted***\n") :
                                                              printf("NAVIC not blacklisted\n");
    }
    else if(result == LE_NOT_PERMITTED)
    {
        printf("GNSS is not in ready or active state!\n");
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets secondary band constellations.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int RequestSecondaryBandConstellations
(
    void
)
{
    uint32_t secondaryBandMask = 0;
    le_result_t result = le_gnss_RequestSecondaryBandConstellations(&secondaryBandMask);

    if (result == LE_OK)
    {
        printf("secondary band constellation %d\n", secondaryBandMask);
        if(secondaryBandMask & (1<<(LE_GNSS_SB_CONSTELLATION_GPS-1)))
        {
            printf("GPS constellation is disabled \n");
        }
        if(secondaryBandMask & (1<<(LE_GNSS_SB_CONSTELLATION_GALILEO-1)))
        {
            printf("GALILEO constellation is disabled \n");
        }
        if(secondaryBandMask & (1<<(LE_GNSS_SB_CONSTELLATION_SBAS-1)))
        {
            printf("SBAS constellation is disabled \n");
        }
        if(secondaryBandMask & (1<<(LE_GNSS_SB_CONSTELLATION_GLONASS-1)))
        {
            printf("GLONASS constellation is disabled \n");
        }
        if(secondaryBandMask &(1<<(LE_GNSS_SB_CONSTELLATION_BDS-1)))
        {
            printf("BDS constellation is disabled \n");
        }
        if(secondaryBandMask & (1<<(LE_GNSS_SB_CONSTELLATION_QZSS-1)))
        {
            printf("QZAS constellation is disabled \n");
        }
        if(secondaryBandMask &(1<<(LE_GNSS_SB_CONSTELLATION_NAVIC-1)))
        {
            printf("NAVIC constellation is disabled \n");
        }
    }
    else if(result == LE_NOT_PERMITTED)
    {
        printf("GNSS is not in ready state!\n");
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets Robust Location Information.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int RobustLocationInformation
(
    void
)
{
    uint8_t enable;
    uint8_t enabled911;
    uint8_t majorVersion;
    uint8_t minorVersion;
    le_result_t result = le_gnss_RobustLocationInformation(&enable,&enabled911,
                             &majorVersion,&minorVersion);

    if (result == LE_OK)
    {
        printf("Robust Location Information Enable: %d\n", enable);
        printf("Robust Location Information enabled911: %d\n", enabled911);
        printf("Robust Location Information majorVersion number: %d\n", majorVersion);
        printf("Robust Location Information minorVersion number: %d\n", minorVersion);
    }
    else if(result == LE_NOT_PERMITTED)
    {
        printf("GNSS is not in ready/active state!\n");
    }
    else
    {
        printf("Failed to get robust location information\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#if 0
//-------------------------------------------------------------------------------------------------
/**
 * This function gets the area of each constellation of gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetConstellationArea
(
    void
)
{
    le_gnss_ConstellationArea_t constellationArea;
    le_gnss_Constellation_t constType = LE_GNSS_SV_CONSTELLATION_GPS;
    le_result_t result;
    static const char *tabConstellation[] =
    {
        "UNDEFINED CONSTELLATION",
        "GPS CONSTELLATION",
        "SBAS CONSTELLATION",
        "GLONASS CONSTELLATION ",
        "GALILEO CONSTELLATION",
        "BEIDOU CONSTELLATION",
        "QZSS CONSTELLATION",
    };

    do
    {
        result = le_gnss_GetConstellationArea(constType, &constellationArea);
        if (LE_OK == result)
        {
            printf("%s area %d\n", tabConstellation[constType], constellationArea);
        }
        else
        {
            printf("Failed! See log for details!\n");
            return EXIT_FAILURE;
        }
        constType++;
    }
    while (LE_GNSS_SV_CONSTELLATION_MAX != constType);

    return EXIT_SUCCESS;
}

#endif
//-------------------------------------------------------------------------------------------------
/**
 * This function gets gnss device acquisition rate.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetAcquisitionRate
(
    void
)
{
    uint32_t acqRate;
    le_result_t result = le_gnss_GetAcquisitionRate(&acqRate);

    switch (result)
    {
        case LE_OK:
            printf("Acquisition Rate: %ums\n", acqRate);
            break;
        case LE_FAULT:
            printf("Failed to get acquisition rate. See logs for details\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in \"active or ready\" state\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets the GNSS minimum elevation.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetMinElevation
(
    void
)
{
    uint8_t  minElevation;
    le_result_t result = le_gnss_GetMinElevation(&minElevation);

    switch (result)
    {
        case LE_OK:
            printf("Minimum elevation: %d\n", minElevation);
            break;
        case LE_FAULT:
            printf("Failed to get the minimum elevation. See logs for details\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS device is not in \"Ready\" state\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the enabled NMEA sentences.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetNmeaSentences
(
    void
)
{
    le_gnss_NmeaBitMask_t nmeaMask;
    le_result_t result = le_gnss_GetNmeaSentences(&nmeaMask);

    switch (result)
    {
        case LE_OK:
            printf("Enabled NMEA sentences bit mask = %"PRIu64"\n", nmeaMask);
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPGGA)
            {
                printf("\tGPGGA (GPS fix data) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GGA)
            {
                printf("\tGGA enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPGSA)
            {
                printf("\tGPGSA (GPS DOP and active satellites) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPGSV)
            {
                printf("\tGPGSV (GPS satellites in view) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPRMC)
            {
                printf("\tGPRMC (GPS recommended minimum data) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_RMC)
            {
                printf("\tRMC enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPVTG)
            {
                printf("\tGPVTG (GPS vector track and speed over the ground) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_VTG)
            {
                printf("\tVTG enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPGNS)
            {
                printf("\tGPGNS enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GNS)
            {
                printf("\tGNS enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GLGSV)
            {
                printf("\tGLGSV (GLONASS satellites in view) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GNGNS)
            {
                printf("\tGNGNS (GNSS fix data) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GNGSA)
            {
                printf("\tGNGSA (GNSS DOP and active satellites) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GSA)
            {
                printf("\tGSA enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GAGGA)
            {
                printf("\tGAGGA (Galileo fix data) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GAGSA)
            {
                printf("\tGAGSA (Galileo DOP and active satellites) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GAGSV)
            {
                printf("\tGAGSV (Galileo satellites in view) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GQGSV)
            {
                printf("\tGQGSV enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GBGSV)
            {
                printf("\tGBGSV enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GIGSV)
            {
                printf("\tGIGSV enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GARMC)
            {
                printf("\tGARMC (Galileo recommended minimum data) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GAVTG)
            {
                printf("\tGAVTG (Galileo vector track and speed over the ground) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_PSTIS)
            {
                printf("\tPSTIS (GPS session start indication) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_PQXFI)
            {
                printf("\tPQXFI (Proprietary Qualcomm eXtended Fix Information) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_PTYPE)
            {
                printf("\tPTYPE (Proprietary Type mask) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPGRS)
            {
                printf("\tGPGRS (GPS Range residuals) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPGLL)
            {
                printf("\tGPGLL (GPS Geographic position, latitude / longitude) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_DEBUG)
            {
               printf("\tDEBUG (Debug NMEA indication) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPDTM)
            {
               printf("\tGPDTM (Local geodetic datum and datum offset from a reference) enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_DTM)
            {
               printf("\tDTM enabled\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GAGNS)
            {
               printf("\tGAGNS (Fix data for Galileo) enabled\n");
            }
            break;
        case LE_FAULT:
            printf("Failed to get enabled NMEA sentences. See logs for details\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS is neither ready nor active state!\n");
            break;
        default:
            printf("Failed to get enabled NMEA sentences, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the Supported NMEA sentences.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetSupportedNmeaSentences
(
    void
)
{
    le_gnss_NmeaBitMask_t nmeaMask;
    le_result_t result = le_gnss_GetSupportedNmeaSentences(&nmeaMask);

    switch (result)
    {
        case LE_OK:
            printf("Supported NMEA sentences bit mask = %"PRIu64"\n", nmeaMask);
            if (nmeaMask & LE_GNSS_NMEA_MASK_GPGSV)
            {
                printf("\tGPGSV (GPS satellites in view) Supported\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GLGSV)
            {
                printf("\tGLGSV (GLONASS satellites in view) Supported\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GAGSV)
            {
                printf("\tGAGSV (Galileo satellites in view) Supported\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GQGSV)
            {
                printf("\tGQGSV Supported\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GBGSV)
            {
                printf("\tGBGSV Supported\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GIGSV)
            {
                printf("\tGIGSV Supported\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GGA)
            {
                printf("\tGGA Supported\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_RMC)
            {
                printf("\tRMC Supported\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GSA)
            {
                printf("\tGSA Supported\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_VTG)
            {
                printf("\tVTG Supported\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_GNS)
            {
                printf("\tGNS Supported\n");
            }
            if (nmeaMask & LE_GNSS_NMEA_MASK_DTM)
            {
                printf("\tDTM Supported\n");
            }
            break;
        case LE_FAULT:
            printf("Failed to get Supported NMEA sentences. See logs for details\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS is not in Ready or Active state!\n");
            break;
        default:
            printf("Failed to get Supported NMEA sentences, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the Supported Constellations
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetSupportedConstellations
(
    void
)
{
    le_gnss_ConstellationBitMask_t constMask;
    le_result_t result = le_gnss_GetSupportedConstellations(&constMask);

    switch (result)
    {
        case LE_OK:
            printf("Supported Constellations bit mask = 0x%08X\n", constMask);
            if (constMask & LE_GNSS_CONSTELLATION_GLONASS)
            {
                printf("\tGLONASS is Supported\n");
            }
            if (constMask & LE_GNSS_CONSTELLATION_BEIDOU)
            {
                printf("\tBEDIDOU is Supported\n");
            }
            if (constMask & LE_GNSS_CONSTELLATION_GALILEO)
            {
                printf("\tGALILEO is Supported\n");
            }
            if (constMask & LE_GNSS_CONSTELLATION_SBAS)
            {
                printf("\tSBAS is Supported\n");
            }
            if (constMask & LE_GNSS_CONSTELLATION_QZSS)
            {
                printf("\tQZSS is Supported\n");
            }
            if (constMask & LE_GNSS_CONSTELLATION_NAVIC)
            {
                printf("\tNAVIC is Supported\n");
            }
            break;
        case LE_FAULT:
            printf("Failed to get Supported Constellations. See logs for details\n");
            break;
        case LE_BUSY:
            printf("Failed to get Supported Constellations, service is busy\n");
            break;
        case LE_TIMEOUT:
            printf("Failed to get Supported Constellations, timeout error\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS is not in ready state!\n");
            break;
        default:
            printf("Failed to get Supported Constellations, error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function configures default second band constellations
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int DefaultSecondaryBandConstellations
(
    void
)
{
    le_result_t result = le_gnss_DefaultSecondaryBandConstellations();

    switch (result)
    {
        case LE_OK:
            printf("Succesfully configured default second band constellations\n");
            break;
        case LE_FAULT:
            printf("Failed to configure secondary band constellations\n");
            break;
        case LE_NOT_PERMITTED:
            printf("GNSS is not in ready state!\n");
            break;
        default:
            printf("Failure error %d (%s)\n",
                    result, LE_RESULT_TXT(result));
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets position fix for last updated sample.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetPosState
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    le_gnss_FixState_t state;
    le_result_t result = le_gnss_GetPositionState( positionSampleRef,
                                                   &state);
    if (result == LE_OK)
    {
        printf("Position state            : %s\n", (state == LE_GNSS_STATE_FIX_NO_POS)?"No Fix"
                                          : (state == LE_GNSS_STATE_FIX_2D)?"2D Fix"
                                          : (state == LE_GNSS_STATE_FIX_3D)?"3D Fix"
                                          : (state == LE_GNSS_STATE_FIX_ESTIMATED)?"Estimated Fix"
                                          : "Invalid");
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets latitude, longitude and horizontal accuracy of last updated location.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int Get2Dlocation
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    int32_t     latitude;
    int32_t     longitude;
    int32_t     hAccuracy;

    le_result_t result = le_gnss_GetLocation( positionSampleRef,
                                              &latitude,
                                              &longitude,
                                              &hAccuracy);

    if (result == LE_OK)
    {
        printf("Latitude(positive->north) : %.6f\n"
               "Longitude(positive->east) : %.6f\n"
               "hAccuracy                 : %.2fm\n",
                (double)latitude/1e6,
                (double)longitude/1e6,
                (float)hAccuracy/1e2);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("Location invalid [%d, %d, %d]\n",
               latitude,
               longitude,
               hAccuracy);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets latitude and vertical accuracy of last updated location.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetAltitude
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    int32_t altitude;
    int32_t vAccuracy;

    le_result_t result = le_gnss_GetAltitude( positionSampleRef,
                                              &altitude,
                                              &vAccuracy);

    if(result == LE_OK)
    {
        printf("Altitude                  : %.3fm\n"
               "vAccuracy                 : %.1fm\n",
               (float)altitude/1e3,
               (float)vAccuracy/10.0);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("Altitude invalid [%d, %d]\n",
               altitude,
               vAccuracy);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the altitude with respect to the WGS-84 ellipsoid of last updated location.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
#if 0
static int GetAltitudeOnWgs84
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    int32_t altitudeOnWgs84;

    le_result_t result = le_gnss_GetAltitudeOnWgs84(positionSampleRef, &altitudeOnWgs84);

    if (LE_OK == result)
    {
        printf("AltitudeOnWgs84  : %.3fm\n", (float)altitudeOnWgs84/1e3);
    }
    else if (LE_OUT_OF_RANGE == result)
    {
        printf("AltitudeOnWgs84 invalid [%d]\n", altitudeOnWgs84);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif

//-------------------------------------------------------------------------------------------------
/**
 * This function gets gps time of last updated sample.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetGpsTime
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t gpsWeek;
    uint32_t gpsTimeOfWeek;

    le_result_t result = le_gnss_GetGpsTime( positionSampleRef,
                                             &gpsWeek,
                                             &gpsTimeOfWeek);

    if (result == LE_OK)
    {
        printf("GPS time                  : Week %02d, TimeOfWeek %d ms\n",
                gpsWeek,
                gpsTimeOfWeek);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("GPS time invalid [%d, %d]\n",
                gpsWeek,
                gpsTimeOfWeek);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets time of last updated location.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetTime
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint16_t    hours = 0;
    uint16_t    minutes = 0;
    uint16_t    seconds = 0;
    uint16_t    milliseconds = 0;

    le_result_t result = le_gnss_GetTime( positionSampleRef,
                                          &hours,
                                          &minutes,
                                          &seconds,
                                          &milliseconds);

    if (result == LE_OK)
    {
        printf("Time(HH:MM:SS:MS)         : %02d:%02d:%02d:%03d\n",
                hours,
                minutes,
                seconds,
                milliseconds);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("Time invalid %02d:%02d:%02d.%03d\n",
                hours,
                minutes,
                seconds,
                milliseconds);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets Epoch time of last updated location.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetEpochTime
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint64_t epochTime;          ///< Epoch time in milliseconds since Jan. 1, 1970

    le_result_t result = le_gnss_GetEpochTime( positionSampleRef, &epochTime);

    if (LE_OK == result)
    {
        printf("Epoch Time                : %llu ms\n", (unsigned long long int) epochTime);
    }
    else if (LE_OUT_OF_RANGE == result)
    {
        printf("Time invalid %llu ms\n", (unsigned long long int) epochTime);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    int status = (result == LE_OK) ? EXIT_SUCCESS: EXIT_FAILURE;
    return status;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets time accuracy of last updated sample.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetTimeAccuracy
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t timeAccuracy;
    le_result_t result = le_gnss_GetTimeAccuracy( positionSampleRef,
                                                  &timeAccuracy);

    if (result == LE_OK)
    {
        printf("GPS time accuracy         : %dns\n", timeAccuracy);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("GPS time accuracy invalid [%d]\n", timeAccuracy);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets current GPS time, leap seconds, next leap seconds event time and next leap
 * seconds value.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetLeapSeconds
(
    void
)
{
    int32_t currentLeapSec, nextLeapSec;
    uint64_t gpsTimeMs, nextEventMs;
    le_result_t result;

    result = le_gnss_GetLeapSeconds(&gpsTimeMs, &currentLeapSec, &nextEventMs,&nextLeapSec);

    if (LE_OK == result)
    {
        printf("Leap seconds report:\n");

        printf("\tCurrent GPS time: ");
        if (gpsTimeMs != UINT64_MAX)
        {
            printf("%"PRIu64" ms\n", gpsTimeMs);
        }
        else
        {
            printf("\n");
        }

        printf("\tLeap seconds: ");
        if (currentLeapSec != INT32_MAX)
        {
            printf("%"PRIi32" ms\n",currentLeapSec);
        }
        else
        {
            printf("\n");
        }

        printf("\tNext event in: ");
        if (nextEventMs != UINT64_MAX)
        {
            printf("%"PRIu64" ms\n", nextEventMs);
        }
        else
        {
            printf("\n");
        }

        printf("\tNext leap seconds in: ");
        if (nextLeapSec != INT32_MAX)
        {
            printf("%"PRIi32" ms\n",nextLeapSec);
        }
        else
        {
            printf("\n");
        }
    }
    else if (LE_TIMEOUT == result)
    {
        printf("Timeout for getting next leap second event.\n");
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets leap seconds uncertainty value.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetLeapSecondsUnc
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint8_t leapSecondsUnc;
    le_result_t result;

    result = le_gnss_GetLeapSecondsUncertainty(positionSampleRef,&leapSecondsUnc);

    if (LE_OK == result)
    {
        printf("Leap seconds Uncertainty: %u sec\n", leapSecondsUnc);
    }
    else if(LE_OUT_OF_RANGE == result)
    {
        printf("Out of range\n");
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets position sample's UTC leap seconds in advance
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetGpsLeapSeconds
(
   le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint8_t leapSecondsPtr;
    le_result_t result;
    result = le_gnss_GetGpsLeapSeconds(positionSampleRef, &leapSecondsPtr);
    if (LE_OK == result)
    {
        printf("Gps Leap seconds : ");
        if (leapSecondsPtr != UINT8_MAX)
        {
            printf("%"PRIu8"s\n", leapSecondsPtr);
        }
        else
        {
            printf("\n");
        }
    }
    else if(LE_TIMEOUT == result)
    {
        printf("Timeout for getting Gps leap seconds\n");
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets position he position sample's magnetic deviation
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetMagneticDeviation
(
   le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    int32_t magneticDeviationPtr ;
    le_result_t result;
    result = le_gnss_GetMagneticDeviation(positionSampleRef, &magneticDeviationPtr);
    if (LE_OK == result)
    {
        printf("Magnetic Deviation : ");
        if (magneticDeviationPtr != INT32_MAX)
        {
            printf("%.1f degrees\n", (float)(magneticDeviationPtr/10.0));
        }
        else
        {
            printf("\n");
        }
    }
    else if(LE_TIMEOUT == result)
    {
        printf("Timeout for getting Magnetic Deviation\n");
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int GetEllipticalUncertainty
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t horUncEllipseSemiMajorPtr;
    uint32_t horUncEllipseSemiMinorPtr;
    uint8_t  horConfidencePtr;

    le_result_t result = le_gnss_GetEllipticalUncertainty( positionSampleRef,
                                                     &horUncEllipseSemiMajorPtr,
                                                     &horUncEllipseSemiMinorPtr,
                                                     &horConfidencePtr);
    if (result == LE_OK)
    {
        printf("HorizontalUncertainty SemiMajor: %.2fm/s\n",(float)horUncEllipseSemiMajorPtr);
        printf("HorizontalUncertainty SemiMinor: %.2fm/s\n",(float)horUncEllipseSemiMinorPtr);
        printf("Horizontal Confidence level: %d%%\n",horConfidencePtr);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("HorizontalUncertainty invalid [%u, %u %u]\n",
                horUncEllipseSemiMajorPtr,
                horUncEllipseSemiMinorPtr,
                horConfidencePtr);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets conformity index for robust location.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetComformingIndex
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    double indexPtr;
    le_result_t result = le_gnss_GetConformityIndex(positionSampleRef,&indexPtr);
    if (result == LE_OK)
    {
        printf("ComformingIndex: %.2f\n" "(0.0->Least conforming 1.0->Most conforming)\n"
                  ,(float)indexPtr);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("GetComformingIndex is invalid\n");
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets sensor calibration status and confidence percent.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetCablibrationConfData
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t calibPtr;
    uint8_t percentPtr;
    le_result_t result = le_gnss_GetCalibrationData(positionSampleRef,
                                                &calibPtr,&percentPtr);
    if (result == LE_OK)
    {
        if(calibPtr & (1<<TAF_LOCGNSS_DR_ROLL_CALIBRATION_NEEDED))
        {
            printf("Roll calibration is needed\n");
        }
        if(calibPtr & (1<<TAF_LOCGNSS_DR_PITCH_CALIBRATION_NEEDED))
        {
            printf("Pitch calibration is needed\n");
        }
        if(calibPtr & (1<<TAF_LOCGNSS_DR_YAW_CALIBRATION_NEEDED))
        {
            printf("Yaw calibration is needed\n");
        }
        if(calibPtr & (1<<TAF_LOCGNSS_DR_ODO_CALIBRATION_NEEDED))
        {
            printf("Odo calibration is needed\n");
        }
        if(calibPtr & (1<<TAF_LOCGNSS_DR_GYRO_CALIBRATION_NEEDED))
        {
            printf("Gyro calibration is needed\n");
        }
        if(calibPtr == 0)
        {
            printf("Calibration status not found\n");
        }
        printf("Sensor calibration confidence percent: %u%%\n"
                  ,percentPtr);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("Calibration data is invalid\n");
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the Dead Reckoning sensor solution status.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetDrSolutionStatus
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t drSolutionPtr;
    le_result_t result = le_gnss_GetDRSolutionStatus(positionSampleRef,
                                                     &drSolutionPtr);
    if (result == LE_OK)
    {
        if(drSolutionPtr & TAF_LOCGNSS_VEHICLE_SENSOR_SPEED_INPUT_DETECTED)
        {
            printf("Vehicle sensor speed input was detected by DRE\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_VEHICLE_SENSOR_SPEED_INPUT_USED)
        {
            printf("Vehicle sensor speed input was used by DRE\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_WARNING_UNCALIBRATED)
        {
            printf("DRE solution disengaged due to insufficient calibration\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_WARNING_GNSS_QUALITY_INSUFFICIENT)
        {
            printf("DRE solution disengaged due to bad GNSS quality\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_WARNING_FERRY_DETECTED)
        {
            printf("DRE solution disengaged as ferry condition detected.\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_ERROR_6DOF_SENSOR_UNAVAILABLE)
        {
            printf("DRE solution disengaged as 6DOF sensor inputs not available\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_ERROR_VEHICLE_SPEED_UNAVAILABLE)
        {
            printf("DRE solution disengaged as vehicle speed inputs not available\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_ERROR_GNSS_EPH_UNAVAILABLE)
        {
            printf("DRE solution disengaged as Ephemeris info not available\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_ERROR_GNSS_MEAS_UNAVAILABLE)
        {
            printf("DRE solution disengaged as GNSS measurement info not available\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_WARNING_INIT_POSITION_INVALID)
        {
            printf("DRE solution disengaged due to non-availability of stored position from previous session\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_WARNING_INIT_POSITION_UNRELIABLE)
        {
            printf("DRE solution disengaged due to vehicle motion detected at session start\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_WARNING_POSITON_UNRELIABLE)
        {
            printf("DRE solution disengaged due to unreliable position\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_ERROR_GENERIC)
        {
            printf("DRE solution disengaged due to a generic error\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_WARNING_SENSOR_TEMP_OUT_OF_RANGE)
        {
            printf("DRE solution disengaged due to Sensor Temperature being out of range\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_WARNING_USER_DYNAMICS_INSUFFICIENT)
        {
            printf("DRE solution disengaged due to insufficient user dynamics\n");
        }
        if(drSolutionPtr & TAF_LOCGNSS_WARNING_FACTORY_DATA_INCONSISTENT)
        {
            printf("DRE solution disengaged due to inconsistent factory data\n");
        }
        if(drSolutionPtr == 0)
        {
            printf("Dead Reckoning solution status not found\n");
        }
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("Dr solution status data is invalid\n");
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the date of updated location.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetDate
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint16_t    year = 0;
    uint16_t    month = 0;
    uint16_t    day = 0;

    le_result_t result = le_gnss_GetDate( positionSampleRef,
                                         &year,
                                         &month,
                                         &day);

    if (result == LE_OK)
    {
        printf("Date(YYYY-MM-DD)          : %04d-%02d-%02d\n",
                year,
                month,
                day);

    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("Date invalid %04d-%02d-%02d\n",
                year,
                month,
                day);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets horizontal speed and its accuracy of last updated sample.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetHorizontalSpeed
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t hSpeed;
    uint32_t hSpeedAccuracy;

    le_result_t result = le_gnss_GetHorizontalSpeed( positionSampleRef,
                                                     &hSpeed,
                                                     &hSpeedAccuracy);
    if (result == LE_OK)
    {
        printf("hSpeed        : %.2fm/s\n"
               "Accuracy      : %.1fcm/s\n",
                hSpeed/100.0,
                hSpeedAccuracy/10.0);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("hSpeed invalid [%u, %u]\n",
                hSpeed,
                hSpeedAccuracy);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets vertical speed and its accuracy of last updated sample.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetVerticalSpeed
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    int32_t vSpeed;
    int32_t vSpeedAccuracy;

    le_result_t result = le_gnss_GetVerticalSpeed( positionSampleRef,
                                                   &vSpeed,
                                                   &vSpeedAccuracy);
    if (result == LE_OK)
    {
        printf( "vSpeed        : %.2fm/s\n"
                "Accuracy      : %.1fcm/s\n",
                vSpeed/100.0,
                vSpeedAccuracy/10.0);
    }
    else if (result == LE_OUT_OF_RANGE)
    {
        printf("vSpeed invalid [%d, %d]\n",
                vSpeed,
                vSpeedAccuracy);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}


//-------------------------------------------------------------------------------------------------
/**
 * This function gets direction of gnss device.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetDirection
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t direction         = 0;
    uint32_t directionAccuracy = 0;

    le_result_t result = le_gnss_GetDirection(positionSampleRef,
                                              &direction,
                                              &directionAccuracy);

    if (result == LE_OK)
    {
        printf("Direction(0 degree is True North) : %.1f degrees\n"
               "Accuracy                          : %.1f degrees\n",
                (float)direction/10.0,
                (float)directionAccuracy/10.0);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("Direction invalid [%u, %u]\n",
               direction,
               directionAccuracy);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the DOP (Dilution of Precision).
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetDop
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint16_t dop[LE_GNSS_RES_UNKNOWN] = { 0 };
    bool err = false;
    le_result_t result = LE_FAULT;
    le_gnss_DopType_t dopType = LE_GNSS_PDOP;
    le_gnss_Resolution_t DopRes;

    static const char *tabDop[] =
    {
        "Position dilution of precision (PDOP)    :",
        "Horizontal dilution of precision (HDOP)  :",
        "Vertical dilution of precision (VDOP)    :",
        "Geometric dilution of precision (GDOP)   :",
        "Time dilution of precision (TDOP)        :"
    };

    do
    {
        // Get DOP parameter in all resolutions
        for (DopRes=LE_GNSS_RES_ZERO_DECIMAL; DopRes<LE_GNSS_RES_UNKNOWN; DopRes++)
        {
            if (LE_OK != le_gnss_SetDopResolution(DopRes))
            {
                printf("Failed! See log for details!\n");
                return EXIT_FAILURE;
            }

            result = le_gnss_GetDilutionOfPrecision(positionSampleRef,
                                                    dopType,
                                                    &dop[DopRes]);
            if (LE_OUT_OF_RANGE == result)
            {
                printf("%s invalid %d\n", tabDop[dopType], dop[0]);
                err = true;
                break;
            }
            else if (LE_OK != result)
            {
                printf("Failed! See log for details!\n");
                return EXIT_FAILURE;
            }
        }
        if (LE_OK == result)
        {
            printf("%s [%.1f %.1f %.2f %.3f]\n", tabDop[dopType],
                   (float)dop[LE_GNSS_RES_ZERO_DECIMAL],
                   (float)dop[LE_GNSS_RES_ONE_DECIMAL]/10,
                   (float)dop[LE_GNSS_RES_TWO_DECIMAL]/100,
                   (float)dop[LE_GNSS_RES_THREE_DECIMAL]/1000);
        }
        dopType++;
    }
    while (dopType != LE_GNSS_DOP_LAST);

    return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

void PrintGnssSignalType(uint32_t signalTypeMask) {
   printf("Signals: ");
   if (signalTypeMask & TAF_LOCGNSS_GPS_L1CA) {
     printf("GPS L1CA, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_GPS_L1C) {
     printf("GPS L1C, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_GPS_L2) {
     printf("GPS L2, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_GPS_L5) {
     printf("GPS L5, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_GLONASS_G1) {
     printf("Glonass G1, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_GLONASS_G2) {
     printf("Glonass G2, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_GALILEO_E1) {
     printf("Galileo E1, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_GALILEO_E5A) {
     printf("Galileo E5A, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_GALILIEO_E5B) {
     printf("Galileo E5B, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_BEIDOU_B1) {
     printf("Beidou B1, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_BEIDOU_B2) {
     printf("Beidou B2, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_QZSS_L1CA) {
     printf("QZSS L1CA, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_QZSS_L1S) {
     printf("QZSS L1S, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_QZSS_L2) {
     printf("QZSS L2, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_QZSS_L5) {
     printf("QZSS L5, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_SBAS_L1) {
     printf("SBAS L1, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_BEIDOU_B1I) {
     printf("Beidou B1I, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_BEIDOU_B1C) {
     printf("Beidou B1C, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_BEIDOU_B2I) {
     printf("Beidou B2I, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_BEIDOU_B2AI) {
     printf("Beidou B2AI, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_NAVIC_L5) {
     printf("Navic L5, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_BEIDOU_B2AQ) {
     printf("Beidou B2AQ, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_BEIDOU_B2BI) {
     printf("Beidou B2BI, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_BEIDOU_B2BQ) {
     printf("Beidou B2BQ, ");
   }
   if (signalTypeMask & TAF_LOCGNSS_NAVIC_L1) {
     printf("Navic L1, ");
   }
   if (signalTypeMask == TAF_LOCGNSS_UNKNOWN_SIGNAL_MASK) {
     printf("No signal, ");
   }
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the Satellites Vehicle information.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetSatelliteInfo
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    // Satellites information
    uint16_t satIdPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satIdNumElements = NUM_ARRAY_MEMBERS(satIdPtr);
    le_gnss_Constellation_t satConstPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satConstNumElements = NUM_ARRAY_MEMBERS(satConstPtr);
    bool satUsedPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satUsedNumElements = NUM_ARRAY_MEMBERS(satUsedPtr);
    uint8_t satSnrPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satSnrNumElements = NUM_ARRAY_MEMBERS(satSnrPtr);
    uint16_t satAzimPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satAzimNumElements = NUM_ARRAY_MEMBERS(satAzimPtr);
    uint8_t satElevPtr[LE_GNSS_SV_INFO_MAX_LEN];
    size_t satElevNumElements = NUM_ARRAY_MEMBERS(satElevPtr);
    int i;

    le_result_t result =  le_gnss_GetSatellitesInfo( positionSampleRef,
                                                     satIdPtr,
                                                     &satIdNumElements,
                                                     satConstPtr,
                                                     &satConstNumElements,
                                                     satUsedPtr,
                                                     &satUsedNumElements,
                                                     satSnrPtr,
                                                     &satSnrNumElements,
                                                     satAzimPtr,
                                                     &satAzimNumElements,
                                                     satElevPtr,
                                                     &satElevNumElements);

    if((result == LE_OK)||(result == LE_OUT_OF_RANGE))
    {
        // Satellite Vehicle information
        for(i=0; i<satIdNumElements; i++)
        {
            if((satIdPtr[i] != 0)&&(satIdPtr[i] != UINT8_MAX))
            {
                printf("[%02d] SVid %03d - C%01d - U%d - SNR%02d - Azim%03d - Elev%02d\n"
                        , i
                        , satIdPtr[i]
                        , satConstPtr[i]
                        , satUsedPtr[i]
                        , satSnrPtr[i]
                        , satAzimPtr[i]
                        , satElevPtr[i]);

                if (LE_GNSS_SV_CONSTELLATION_SBAS == satConstPtr[i])
                {
                    //uncomment this, when feature is enabled
                    /*printf("SBAS category : %d\n",
                           le_gnss_GetSbasConstellationCategory(satIdPtr[i]));*/
                }
            }
        }
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the Satellites Vehicle information.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetSatelliteInfoEx
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    // Satellites information
    int i;
    int index = 0;
    le_result_t result;

    for (int constellation = 1; constellation < TAF_LOCGNSS_SV_CONSTELLATION_MAX; constellation++) {

      taf_locGnss_SvInfo_t svInfo[TAF_LOCGNSS_SV_INFO_MAX_SATS_IN_CONSTELLATIONS];
      size_t svInfoLen = TAF_LOCGNSS_SV_INFO_MAX_SATS_IN_CONSTELLATIONS;

      result = le_gnss_GetSatellitesInfoEx(positionSampleRef, constellation, svInfo, &svInfoLen);

      if((result == LE_OK)||(result == LE_OUT_OF_RANGE)||(result == LE_OVERFLOW))
      {
        LE_INFO("gnss tool: result %d, constellation: %d, numOfSvInfo: %d", (int)result, (int) constellation, (int) svInfoLen);
        // Satellite Vehicle information
        for(i=0; i<(int) svInfoLen; i++)
        {
            if((svInfo[i].satId != 0)&&(svInfo[i].satId != UINT8_MAX))
            {
                printf("[%02d] SVid %03d - C%01d - U%d - T%d - SNR%02d - Azim%03d - Elev%02d\n"
                        , index++
                        , svInfo[i].satId
                        , svInfo[i].satConst
                        , svInfo[i].satUsed
                        , svInfo[i].satTracked
                        , svInfo[i].satSnr
                        , svInfo[i].satAzim
                        , svInfo[i].satElev);

                PrintGnssSignalType(svInfo[i].signalType);
                printf("Glonass FCN: %d\n", svInfo[i].glonassFcn);
                printf("Baseband Carrier To Noise Ratio: %lfdB-Hz\n", svInfo[i].baseBandCnr);
            }
        }
      }
      else
      {
        printf("Failed for constellation %d, See log for details!\n", (int) constellation);
      }
    }

    return ((result == LE_OK)||(result == LE_OUT_OF_RANGE)||(result == LE_OVERFLOW)) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the Satellites Vehicle status.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetSatelliteStatus
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint8_t satsInViewCount;
    uint8_t satsTrackingCount;
    uint8_t satsUsedCount;
    le_result_t result =  le_gnss_GetSatellitesStatus( positionSampleRef,
                                                       &satsInViewCount,
                                                       &satsTrackingCount,
                                                       &satsUsedCount);

    LE_ASSERT((result == LE_OK)||(result == LE_OUT_OF_RANGE));

    if ((result == LE_OK) || (result == LE_OUT_OF_RANGE))
    {
        printf("SatsInView: %d, SatsTracking: %d and SatsUsed: %d\n",
               (satsInViewCount == UINT8_MAX) ? 0: satsInViewCount,
               (satsTrackingCount == UINT8_MAX) ? 0: satsTrackingCount,
               (satsUsedCount == UINT8_MAX) ? 0: satsUsedCount);
    }
    else
    {
        printf("Failed! See log for details!\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets Kinematics information related to body parameters.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetKinematicsData
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    taf_locGnss_KinematicsData_t *bodyFrameData;
    le_mem_PoolRef_t bodyFramePool = NULL;
    bodyFramePool = le_mem_CreatePool("bodyFramePool", sizeof(taf_locGnss_KinematicsData_t));
    bodyFrameData = (taf_locGnss_KinematicsData_t*) le_mem_ForceAlloc(bodyFramePool);

    le_result_t result = le_gnss_GetBodyFrameData( positionSampleRef,
                                              bodyFrameData);

    if (result == LE_OK)
    {
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_LONG_ACCEL))
       {
           printf("**Kinematics data has Forward Accelaration**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_LAT_ACCEL))
       {
           printf("**Kinematics data has has Sideward Acceleration**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_VERT_ACCEL))
       {
           printf("**Kinematics data has Vertical Acceleration**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_YAW_RATE))
       {
           printf("**Kinematics has data has Heading Rate**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_PITCH))
       {
           printf("**Kinematics has body pitch**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_LONG_ACCEL_UNC))
       {
           printf("**Kinematics data has has Forward Acceleration Uncertainty**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_LAT_ACCEL_UNC))
       {
           printf("**Kinematics data has has Sideward Acceleration Uncertainty**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_VERT_ACCEL_UNC))
       {
           printf("**Kinematics data has Vertical Acceleration Uncertainty**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_YAW_RATE_UNC))
       {
           printf("**Kinematics data Heading rate uncertainity**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_PITCH_UNC))
       {
           printf("**Kinematics has body pitch Uncertainity**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_PITCH_RATE_BIT))
       {
           printf("**Kinematics data has pitch rate**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_PITCH_RATE_UNC_BIT))
       {
           printf("**Kinematics has pitch rate Uncertainity**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_ROLL_BIT))
       {
           printf("**Kinematics data has roll**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_ROLL_UNC_BIT))
       {
           printf("**Kinematics data has roll Uncertainity**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_ROLL_RATE_BIT))
       {
           printf("**Kinematics data has roll rate**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_ROLL_RATE_UNC_BIT))
       {
           printf("**Kinematics data has roll rate Uncertainity**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_YAW_BIT))
       {
           printf("**Kinematics data has yaw**\n");
       }
       if((bodyFrameData->bodyFrameDataMask) & (1<<TAF_LOCGNSS_HAS_YAW_UNC_BIT))
       {
           printf("**Kinematics data has yaw uncertainity**\n");
       }
       printf("\nForward Acceleration in body frame(m/s2):%lf\n",(float)bodyFrameData->longAccel);
       printf("Sideward Acceleration in body frame (m/s2):%lf\n",(float) bodyFrameData->latAccel);
       printf("Vertical Acceleration in body frame (m/s2):%lf\n",(float) bodyFrameData->vertAccel);
       printf("Heading Rate (Radians/second):%lf\n",(float) bodyFrameData->yawRate);
       printf("Body pitch (Radians)::%lf\n",(float) bodyFrameData->pitch);
       printf("Uncertainty of Forward Acceleration in body frame:%lf\n",
                                            (float) bodyFrameData->longAccelUnc);
       printf("Uncertainty of Side-ward Acceleration in body frame:%lf\n",
                                            (float) bodyFrameData->latAccelUnc);
       printf("Uncertainty of Vertical Acceleration in body frame:%lf\n",
                                            (float)bodyFrameData->vertAccelUnc);
       printf("Uncertainty of Heading Rate:%lf\n",(float) bodyFrameData->yawRateUnc);
       printf("Uncertainty of Body pitch:%lf\n",(float) bodyFrameData->pitchUnc);
       printf("Body pitch rate:%lf\n",(float) bodyFrameData->pitchRate);
       printf("Uncertainty of pitch rate:%lf\n",(float) bodyFrameData->pitchRateUnc);
       printf("Roll of body frame, clockwise is positive:%lf\n",(float)bodyFrameData->roll);
       printf("Uncertainty of roll, 68%% confidence level:%lf\n",(float)bodyFrameData->rollUnc);
       printf("Roll rate of body frame,clockwise is positive:%lf\n",(float)bodyFrameData->rollRate);
       printf("Uncertainty of roll rate, 68%% confidence level:%lf\n",
                                            (float)bodyFrameData->rollRateUnc);
       printf("Yaw of body frame, clockwise is positive:%lf\n",(float)bodyFrameData->yaw);
       printf("Uncertainty of yaw, 68%% confidence level:%lf\n",(float)bodyFrameData->yawUnc);

    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("Kinematics data is not found");
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    le_mem_Release(bodyFrameData);

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * Function to get VRP based latitude, longitude & altitude information.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetVRPBasedLocation
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    double latitude;
    double longitude;
    double altitude;

    le_result_t result = le_gnss_GetVRPBasedLLA( positionSampleRef,
                                              &latitude,
                                              &longitude,
                                              &altitude);

    if (result == LE_OK)
    {
        printf("VRP based Latitude(positive->north) : %lf degrees\n"
               "VRP based Longitude(positive->east) : %lf degress\n"
               "VRP based altitude                  : %lfm\n",
                latitude,
                longitude,
                (float)altitude);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("GetVRPBasedLocation invalid [%lf, %lf, %lf]\n",
               latitude,
               longitude,
               altitude);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;

}

//-------------------------------------------------------------------------------------------------
/**
 * Function to get VRP based east, north & up velocity information.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetVRPBasedVelocityInfo
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    double eastVel;
    double northVel;
    double upVel;

    le_result_t result = le_gnss_GetVRPBasedVelocity( positionSampleRef,
                                              &eastVel,
                                              &northVel,
                                              &upVel);

    if (result == LE_OK)
    {
        printf("VRP based east velocity  : %lf\n"
               "VRP based north velocity : %lf\n"
               "VRP based up velocity    : %lf\n",
                (float)eastVel,
                (float)northVel,
                (float)upVel);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("GetVRPBasedVelocity invalid [%lf, %lf, %lf]\n",
               eastVel,
               northVel,
               upVel);
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;

}

//-------------------------------------------------------------------------------------------------
/**
 * Function to get the set of satellite vehicles that are used to calculate position.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetSvData
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    taf_locGnss_SvUsedInPosition_t *svData;
    le_mem_PoolRef_t svFramePool = NULL;
    svFramePool = le_mem_CreatePool("svFramePool", sizeof(taf_locGnss_SvUsedInPosition_t));
    svData = (taf_locGnss_SvUsedInPosition_t*) le_mem_ForceAlloc(svFramePool);

    le_result_t result = le_gnss_GetSvUsedInPosition( positionSampleRef,
                                                     svData);

    if (result == LE_OK)
    {
        printf("SVs from GPS constellation       : %"PRIu64"\n",svData->gps);
        printf("SVs from GLONASS constellation   : %"PRIu64"\n",svData->glo);
        printf("SVs from GALILEO constellation   : %"PRIu64"\n",svData->gal);
        printf("SVs from BEIDOU constellation    : %"PRIu64"\n",svData->bds);
        printf("SVs from QZSS constellation      : %"PRIu64"\n",svData->qzss);
        printf("SVs from NAVIC constellation     : %"PRIu64"\n",svData->navic);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("GetSvData is invalid\n");
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    le_mem_Release(svData);

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;

}

//-------------------------------------------------------------------------------------------------
/**
 * Function to get navigation solution mask used to indicate SBAS corrections.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetSbasType
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t sbasMask = 0;

    le_result_t result = le_gnss_GetSbasCorrection( positionSampleRef,
                                                     &sbasMask);
    printf("SBAS correction: ");
    if (result == LE_OK)
    {
        if(sbasMask & LE_GNSS_SBAS_CORRECTION_IONO)
        {
            printf("SBAS ionospheric correction is used\n");
        }
        if(sbasMask & LE_GNSS_SBAS_CORRECTION_FAST)
        {
            printf("SBAS fast correction is used\n");
        }
        if(sbasMask & LE_GNSS_SBAS_CORRECTION_LONG)
        {
            printf("SBAS long correction is used\n");
        }
        if(sbasMask & LE_GNSS_SBAS_INTEGRITY)
        {
            printf("SBAS integrity information is used\n");
        }
        if(sbasMask & LE_GNSS_SBAS_CORRECTION_DGNSS)
        {
            printf("SBAS DGNSS correction information is used\n");
        }
        if(sbasMask & LE_GNSS_SBAS_CORRECTION_RTK)
        {
            printf("SBAS RTK correction information is used\n");
        }
        if(sbasMask & LE_GNSS_SBAS_CORRECTION_PPP)
        {
            printf("SBAS PPP correction information is used\n");
        }
        if(sbasMask & LE_GNSS_SBAS_CORRECTION_RTK_FIXED)
        {
            printf("SBAS RTK fixed correction information is used\n");
        }
        if(sbasMask & LE_GNSS_SBAS_CORRECTED_SV_USED)
        {
            printf("SBAS correction SV is used\n");
        }
        if(sbasMask == 0)
        {
            printf("No SBAS corrections data\n");
        }
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("GetSbasType is invalid\n");
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;

}

//-------------------------------------------------------------------------------------------------
/**
 * Function to get position technology mask to indicate which technology is used.
 * The technology used in computing the fix.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetTechInformation
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t techMask;

    le_result_t result = le_gnss_GetPositionTechnology( positionSampleRef,
                                                         &techMask);
    if (result == LE_OK)
    {
        printf("\nTechnology used to compute fix: The ");
        if(techMask & LE_GNSS_LOC_GNSS)
        {
            printf("location calculated using GNSS\n");
        }
        if(techMask & LE_GNSS_LOC_CELL)
        {
            printf("location calculated using CELL\n");
        }
        if(techMask & LE_GNSS_LOC_WIFI)
        {
            printf("location calculated using WIFI\n");
        }
        if(techMask & LE_GNSS_LOC_SENSORS)
        {
            printf("location calculated using SENSORS\n");
        }
        if(techMask & LE_GNSS_LOC_REFERENCE_LOCATION)
        {
            printf("location calculated using reference location\n");
        }
        if(techMask & LE_GNSS_LOC_INJECTED_COARSE_POSITION)
        {
            printf("location calculated using Coarse position injected\n");
        }
        if(techMask & LE_GNSS_LOC_AFLT)
        {
            printf("location calculated using AFLT\n");
        }
        if(techMask & LE_GNSS_LOC_HYBRID)
        {
            printf("location calculated using GNSS and network-provided measurements\n");
        }
        if(techMask & LE_GNSS_LOC_PPE)
        {
            printf("location calculated using Precise position engine\n");
        }
        if(techMask & LE_GNSS_LOC_VEH)
        {
            printf("location calculated using Vehicular data\n");
        }
        if(techMask & LE_GNSS_LOC_VIS)
        {
            printf("location calculated using Visual data\n");
        }
        if(techMask & LE_GNSS_LOC_PROPAGATED)
        {
            printf("location calculated using propagation logic\n");
        }
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("GetTechInformation is invalid\n");
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;

}

//-------------------------------------------------------------------------------------------------
/**
 * Function to get the validity of the Location basic Info.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetValidityInfo
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint32_t validityMask = 0;
    uint64_t validityExMask = 0;

    le_result_t result = le_gnss_GetLocationInfoValidity( positionSampleRef,
                                                        &validityMask,&validityExMask);
    if (result == LE_OK)
    {
        printf("\n** Location Info Validity Information ***\n");
        if(validityMask & LE_GNSS_HAS_LAT_LONG_BIT)
        {
            printf("valid latitude longitude\n");
        }
        if(validityMask & LE_GNSS_HAS_ALTITUDE_BIT)
        {
            printf("valid altitude\n");
        }
        if(validityMask & LE_GNSS_HAS_SPEED_BIT)
        {
            printf("valid speed\n");
        }
        if(validityMask & LE_GNSS_HAS_HEADING_BIT)
        {
            printf("valid heading\n");
        }
        if(validityMask & LE_GNSS_HAS_HORIZONTAL_ACCURACY_BIT)
        {
            printf("valid horizontal accuracy\n");
        }
        if(validityMask & LE_GNSS_HAS_VERTICAL_ACCURACY_BIT)
        {
            printf("valid vertical accuracy\n");
        }
        if(validityMask & LE_GNSS_HAS_SPEED_ACCURACY_BIT)
        {
            printf("valid speed accuracy \n");
        }
        if(validityMask & LE_GNSS_HAS_HEADING_ACCURACY_BIT)
        {
            printf("valid heading accuracy\n");
        }
        if(validityMask & LE_GNSS_HAS_TIMESTAMP_BIT)
        {
            printf("valid timestamp\n");
        }
        if(validityMask & LE_GNSS_HAS_ELAPSED_REAL_TIME_BIT)
        {
            printf("valid elapsed real time\n");
        }
        if(validityMask & LE_GNSS_HAS_ELAPSED_REAL_TIME_UNC_BIT)
        {
            printf("valid elapsed real time Uncertainity\n");
        }
        if(validityMask & LE_GNSS_HAS_GPTP_TIME_BIT)
        {
            printf("valid gptp time\n");
        }
        if(validityMask & LE_GNSS_HAS_GPTP_TIME_UNC_BIT)
        {
            printf("valid gptp time Uncertainity\n");
        }
        if(validityMask == 0)
        {
            printf("no Valid Mask\n");
        }
        printf("\n** Location Info Ex Validity Information ***\n");

        if(validityExMask & (1ULL << LE_GNSS_HAS_ALTITUDE_MEAN_SEA_LEVEL))
        {
            printf("valid altitude mean sea level\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_DOP))
        {
            printf("valid pdop, hdop, vdop\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_MAGNETIC_DEVIATION))
        {
            printf("valid magnetic deviation\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_HOR_RELIABILITY))
        {
            printf("valid horizontal reliability\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_VER_RELIABILITY))
        {
            printf("valid vertical reliability\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_HOR_ACCURACY_ELIP_SEMI_MAJOR))
        {
            printf("valid elipsode semi major\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_HOR_ACCURACY_ELIP_SEMI_MINOR))
        {
            printf("valid elipsode semi minor\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_HOR_ACCURACY_ELIP_AZIMUTH))
        {
            printf("valid accuracy elipsode azimuth\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_GNSS_SV_USED_DATA))
        {
            printf("valid gnss sv used in pos data\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_NAV_SOLUTION_MASK))
        {
            printf("valid navSolutionMask\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_POS_TECH_MASK))
        {
            printf("valid LocPosTechMask\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_SV_SOURCE_INFO))
        {
            printf("valid LocSvInfoSource\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_POS_DYNAMICS_DATA))
        {
            printf("valid position dynamics data\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_EXT_DOP))
        {
            printf("valid gdop, tdop\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_NORTH_STD_DEV))
        {
            printf("valid North standard deviation\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_EAST_STD_DEV))
        {
            printf("valid East standard deviation\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_NORTH_VEL))
        {
            printf("valid North Velocity\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_EAST_VEL))
        {
            printf("valid East Velocity""\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_UP_VEL))
        {
            printf("valid Up Velocity\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_NORTH_VEL_UNC))
        {
            printf("valid North Velocity Uncertainty\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_EAST_VEL_UNC))
        {
            printf("valid East Velocity Uncertainty\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_UP_VEL_UNC))
        {
            printf("valid Up Velocity Uncertainty\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_LEAP_SECONDS))
        {
            printf("valid leap_seconds\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_TIME_UNC))
        {
            printf("valid timeUncMs\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_NUM_SV_USED_IN_POSITION))
        {
            printf("valid number of sv used\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_CALIBRATION_CONFIDENCE_PERCENT))
        {
            printf("valid sensor calibrationConfidencePercent\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_CALIBRATION_STATUS))
        {
            printf("valid sensor calibrationConfidence\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_OUTPUT_ENG_TYPE))
        {
            printf("valid output engine type\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_OUTPUT_ENG_MASK))
        {
            printf("valid output engine mask\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_CONFORMITY_INDEX_FIX))
        {
            printf("valid conformity index\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_LLA_VRP_BASED))
        {
            printf("valid lla vrp based\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_ENU_VELOCITY_VRP_BASED))
        {
            printf("valid enu velocity vrp based\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_ALTITUDE_TYPE))
        {
            printf("valid altitude type\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_REPORT_STATUS))
        {
            printf("valid report status\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_INTEGRITY_RISK_USED))
        {
            printf("valid integrity risk\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_PROTECT_LEVEL_ALONG_TRACK))
        {
            printf("valid protect along track\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_PROTECT_LEVEL_CROSS_TRACK))
        {
            printf("valid protect cross track\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_PROTECT_LEVEL_VERTICAL))
        {
            printf("valid protect vertical\n");
        }
        if(validityExMask & (1ULL << LE_GNSS_HAS_LEAP_SECONDS_UNC))
        {
            printf("valid leap seconds uncertainty\n");
        }
        if(validityExMask == 0)
        {
            printf("no ValidEx Mask\n");
        }
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("GetValidityInfo is invalid\n");
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;

}

//-------------------------------------------------------------------------------------------------
/**
 * Function to get the combination of position engines and location engine type
 * used in calculating the position report.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetEngineOutputParams
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint16_t engMask = 0;
    uint16_t locationEngType = 0;

    le_result_t result = le_gnss_GetLocationOutputEngParams(positionSampleRef,
                                                        &engMask,&locationEngType);
    if (result == LE_OK)
    {
        if(locationEngType == LE_GNSS_LOC_OUTPUT_ENGINE_FUSED)
        {
            printf("This is FUSED engine reports\n");
        }
        if(locationEngType == LE_GNSS_LOC_OUTPUT_ENGINE_SPE)
        {
            printf("This is SPE engine reports\n");
        }
        if(locationEngType == LE_GNSS_LOC_OUTPUT_ENGINE_PPE)
        {
            printf("This is PPE engine reports\n");
        }
        if(locationEngType == LE_GNSS_LOC_OUTPUT_ENGINE_VPE)
        {
            printf("This is VPE engine reports\n");
        }
        if(engMask & LE_GNSS_STANDARD_POSITIONING_ENGINE)
        {
            printf("SPE used in the reports\n");
        }
        if(engMask & LE_GNSS_DEAD_RECKONING_ENGINE)
        {
            printf("DRE used in the reports\n");
        }
        if(engMask & LE_GNSS_PRECISE_POSITIONING_ENGINE)
        {
            printf("PPE used in the reports\n");
        }
        if(engMask & LE_GNSS_VP_POSITIONING_ENGINE)
        {
            printf("VPE used in the reports\n");
        }
        if(engMask == 0)
        {
            printf("no output engine Mask Mask\n");
        }
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("GetEngineOutputParams is invalid\n");
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;

}

//-------------------------------------------------------------------------------------------------
/**
 * Function to get the reliability of the horizontal & vertical positions.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetReliabilityInfo
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint16_t horiReliablity = 0;
    uint16_t vertReliablity = 0;

    le_result_t result = le_gnss_GetReliabilityInformation(positionSampleRef,
                                                        &horiReliablity,&vertReliablity);
    if (result == LE_OK)
    {
        if(horiReliablity & LE_GNSS_RELIABILITY_NOT_SET)
        {
            printf("Horizontal reliability: NOT_SET\n");
        }
        else if(horiReliablity & LE_GNSS_RELIABILITY_VERY_LOW)
        {
            printf("Horizontal reliability: VERY_LOW\n");
        }
        else if(horiReliablity & LE_GNSS_RELIABILITY_LOW)
        {
            printf("Horizontal reliability: LOW\n");
        }
        else if(horiReliablity & LE_GNSS_RELIABILITY_MEDIUM)
        {
            printf("Horizontal reliability: MEDIUM\n");
        }
        else if(horiReliablity & LE_GNSS_RELIABILITY_HIGH)
        {
            printf("Horizontal reliability: HIGH\n");
        }
        else
        {
            printf("Horizontal reliability: UNKNOWN\n");
        }
        if(vertReliablity & LE_GNSS_RELIABILITY_NOT_SET)
        {
            printf("Vertical reliability  : NOT_SET\n");
        }
        else if(vertReliablity & LE_GNSS_RELIABILITY_VERY_LOW)
        {
            printf("Vertical reliability  : VERY_LOW\n");
        }
        else if(vertReliablity & LE_GNSS_RELIABILITY_LOW)
        {
            printf("Vertical reliability  : LOW\n");
        }
        else if(vertReliablity & LE_GNSS_RELIABILITY_MEDIUM)
        {
            printf("Vertical reliability  : MEDIUM\n");
        }
        else if(vertReliablity & LE_GNSS_RELIABILITY_HIGH)
        {
            printf("Vertical reliability  : HIGH\n");
        }
        else
        {
            printf("Vertical reliability  : UNKNOWN\n");
        }
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("GetReliabilityInfo is invalid\n");
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;

}

//-------------------------------------------------------------------------------------------------
/**
 * Function to get the elliptical horizontal uncertainty azimuth of orientation,
 * east and north standard deviations.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetAzimuthDevInfo
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    double azimuth;
    double eastDev;
    double northDev;

    le_result_t result = le_gnss_GetStdDeviationAzimuthInfo(positionSampleRef,
                                                        &azimuth,&eastDev,&northDev);
    if (result == LE_OK)
    {
        printf("Azimuth                  : %lf degrees\n",(float)azimuth);
        printf("East standard deviation  : %lfm\n",(float)eastDev);
        printf("North standard deviation : %lfm\n",(float)northDev);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("GetAzimuthDevInfo is invalid\n");
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;

}

//-------------------------------------------------------------------------------------------------
/**
 * Function to get the elliptical horizontal uncertainty azimuth of orientation,
 * east and north standard deviations.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetRealTimeInfo
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint64_t realTime;
    uint64_t realTimeUnc;

    le_result_t result = le_gnss_GetRealTimeInformation(positionSampleRef,
                                                        &realTime,&realTimeUnc);
    if (result == LE_OK)
    {
        printf("Elapsed real time              : %"PRIu64" ns\n",realTime);
        printf("Elapsed real time uncertainity : %"PRIu64" ns\n",realTimeUnc);
    }
    else if(result == LE_OUT_OF_RANGE)
    {
        printf("GetRealTimeInfo is invalid\n");
    }
    else
    {
        printf("Failed! See log for details\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;

}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets gnss meaurement usage info.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetGnssMeasurementInfo
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    taf_locGnss_GnssMeasurementInfo_t measInfo[TAF_LOCGNSS_MEASUREMENT_INFO_MAX];
    size_t gnssMeasLen = TAF_LOCGNSS_MEASUREMENT_INFO_MAX;
    le_result_t result = le_gnss_GetMeasurementUsageInfo(positionSampleRef, measInfo, &gnssMeasLen);

    if (result != LE_OK)
    {
      printf("Error to get gnss measurement info.\n");
      return EXIT_FAILURE;
    }

   for(uint16_t i = 0; i < gnssMeasLen; i++) {
      taf_locGnss_GnssSystem_t system = measInfo[i].gnssConstellation;
      printf("System: ");
      if(system == TAF_LOCGNSS_LOC_SV_SYSTEM_GPS) {
         printf("GPS, ");
      }
      else if(system == TAF_LOCGNSS_LOC_SV_SYSTEM_GALILEO) {
         printf("GALILEO, ");
      }
      else if(system == TAF_LOCGNSS_LOC_SV_SYSTEM_SBAS) {
         printf("SBAS, ");
      }
      else if(system == TAF_LOCGNSS_LOC_SV_SYSTEM_GLONASS) {
         printf("GLONASS, ");
      }
      else if(system == TAF_LOCGNSS_LOC_SV_SYSTEM_BDS) {
         printf("BDS, ");
      }
      else if(system == TAF_LOCGNSS_LOC_SV_SYSTEM_QZSS) {
         printf("QZSS, ");
      }
      else if(system == TAF_LOCGNSS_LOC_SV_SYSTEM_NAVIC) {
         printf("NAVIC, ");
      }
      else {
         printf("UNKNOWN, ");
      }

      uint32_t signalType = measInfo[i].gnssSignalType;

      PrintGnssSignalType(signalType);

      printf("GNSS SV ID: %d\n", measInfo[i].gnssSvId);
   }
   return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

void printReportStatus(taf_locGnss_ReportStatus_t status) {
  printf("Report Status is: ");
  if (status == TAF_LOCGNSS_REPORT_STATUS_UNKNOWN) {
    printf("UNKNOWN\n");
  }
  if (status == TAF_LOCGNSS_REPORT_STATUS_SUCCESS) {
    printf("SUCCESS\n");
  }
  if (status == TAF_LOCGNSS_REPORT_STATUS_INTERMEDIATE) {
    printf("INTERMEDIATE\n");
  }
  if (status == TAF_LOCGNSS_REPORT_STATUS_FAILURE) {
    printf("FAILURE\n");
  }
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets status of report in terms of how optimally the report was calculated by engine.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetReportStatus
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    int32_t reportStatus = -1;
    le_result_t result = le_gnss_GetReportStatus(positionSampleRef, &reportStatus);

    if (result != LE_OK)
    {
      printf("Error to get report status\n");
      return EXIT_FAILURE;
    }

    printReportStatus((taf_locGnss_ReportStatus_t) reportStatus);

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets the altitude with respect to mean sea level in meters.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetAltitudeMeanSeaLevel
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    double altMSeaLevel;
    le_result_t result = le_gnss_GetAltitudeMeanSeaLevel(positionSampleRef, &altMSeaLevel);

    if (result != LE_OK)
    {
      printf("Error to get AltitudeMeanSeaLevel\n");
      return EXIT_FAILURE;
    }

    printf("Altitude with respect to mean sea level: %lfm\n",(float)altMSeaLevel);

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function gets GNSS Satellite Vehicles used in position data.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetSVIds
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    uint16_t svIds[TAF_LOCGNSS_MEASUREMENT_INFO_MAX];
    size_t svIdsLen = TAF_LOCGNSS_MEASUREMENT_INFO_MAX;
    le_result_t result = le_gnss_GetSVIds(positionSampleRef, svIds, &svIdsLen);

    if (result != LE_OK)
    {
      printf("Error to get sv Ids.\n");
      return EXIT_FAILURE;
    }
    if (svIdsLen > 0)
        printf("Ids of used SVs:");
    else
        printf("No SVIDs found\n");
    for(uint16_t i = 0; i < svIdsLen; i++) {
        printf(" %d", svIds[i]);
    }
    if (svIdsLen > 0) printf("\n");

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int GetGnssData
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    size_t maxSigTypes = TAF_LOCGNSS_NUMBER_OF_SIGNAL_TYPES_MAX;
    taf_locGnss_GnssData_t gnssDataPtr[TAF_LOCGNSS_NUMBER_OF_SIGNAL_TYPES_MAX];
    le_result_t result = le_gnss_GetGnssData(positionSampleRef, gnssDataPtr, &maxSigTypes);

    if (result != LE_OK)
    {
      printf("Error to get Jammer and AGC data.\n");
      return EXIT_FAILURE;
    }

    for(uint8_t i = 0; i < maxSigTypes; i++)
    {
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_GPS_L1CA)
        {
            printf("Signal type is :GPS L1CA\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_GPS_L1C)
        {
            printf("Signal type is :GPS L1C\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_GPS_L2C_L)
        {
            printf("Signal type is :GPS L2\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_GPS_L5_Q)
        {
            printf("Signal type is :GPS L5\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_GLONASS_G1)
        {
            printf("Signal type is :GLONASS G1\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_GLONASS_G2)
        {
            printf("Signal type is :GLONASS G2\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_GALILEO_E1_C)
        {
            printf("Signal type is :GALILEO E1\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_GALILEO_E5A_Q)
        {
            printf("Signal type is :GALILEO E5A\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_GALILEO_E5B_Q)
        {
            printf("Signal type is :GALILEO E5B\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_BEIDOU_B1_I)
        {
            printf("Signal type is :BEIDOU B1_I\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_BEIDOU_B1C)
        {
            printf("Signal type is :BEIDOU B1_C\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_BEIDOU_B2_I)
        {
            printf("Signal type is :BEIDOU B2_I\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_BEIDOU_B2A_I)
        {
            printf("Signal type is :BEIDOU B2_AI\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_QZSS_L1CA)
        {
            printf("Signal type is :QZSS L1CA\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_QZSS_L1S)
        {
            printf("Signal type is :QZSS L1S\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_QZSS_L2C_L)
        {
            printf("Signal type is :QZSS L2CL\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_QZSS_L5_Q)
        {
            printf("Signal type is :QZSS L5Q\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_SBAS_L1_CA)
        {
            printf("Signal type is :SBAS L1CA\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_NAVIC_L5)
        {
            printf("Signal type is :NAVIC L5\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_BEIDOU_B2A_Q)
        {
            printf("Signal type is :BEIDOU B2AQ\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_BEIDOU_B2BI)
        {
            printf("Signal type is :BEIDOU B2BI\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_BEIDOU_B2BQ)
        {
            printf("Signal type is :BEIDOU B2BQ\n");
        }
        if(i == TAF_LOCGNSS_DATA_SIGNAL_TYPE_NAVIC_L1)
        {
            printf("Signal type is :NAVIC L1\n");
        }
        printf("gnssDataMask:%d\n", gnssDataPtr[i].gnssDataMask);
        if(gnssDataPtr[i].gnssDataMask & TAF_LOCGNSS_HAS_JAMMER)
        {
            printf("jammerInd is present\n");
            printf("jammerInd: %lf\n",gnssDataPtr[i].jammerInd);
        }
        else
        {
            printf("jammerInd is not present\n");
        }
        if(gnssDataPtr[i].gnssDataMask & TAF_LOCGNSS_HAS_AGC)
        {
            printf("Automatic Gain Control is present\n");
            printf("AGC: %lf\n",gnssDataPtr[i].agc);
        }
        else
        {
            printf("Automatic Gain Control is not present\n");
        }
        printf("\n");
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/**
 * This function Sets the dead reckoning parameters validity mask.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int SetDRValidityMask
(
    const char* validityMaskPtr           ///< [IN] DR validity mask
)
{
    char *end;
    uint32_t mask = strtoul(validityMaskPtr, &end, BASE10);

    if ('\0' != end[0])
    {
        printf("Bad DR validity mask : %s\n", validityMaskPtr);
        return EXIT_FAILURE;
    }

    le_result_t result = le_gnss_SetDRConfigValidity(mask);

    switch (result)
    {
        case LE_OK:
            printf("Successfully set DR engine parameters mask!\n");
            break;
        case LE_FAULT:
            printf("Failed to set DR engine parameters mask\n");
            break;
        case LE_BAD_PARAMETER:
            printf("Bad parameter to set DR engine parameters mask\n");
            break;
        case LE_NOT_PERMITTED:
            printf("The GNSS device is not ready state\n");
            break;
        default:
            printf("Invalid status\n");
            break;
    }

    return (LE_OK == result) ? EXIT_SUCCESS : EXIT_FAILURE;
}
//-------------------------------------------------------------------------------------------------
/**
 * Function to get all positional information of last updated sample.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int GetPosInfo
(
    le_gnss_SampleRef_t positionSampleRef    ///< [IN] Position sample reference
)
{
    int status = EXIT_SUCCESS;
    status = (EXIT_FAILURE == GetTtff(le_gnss_GetState())) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetPosState(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == Get2Dlocation(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetAltitude(positionSampleRef)) ? EXIT_FAILURE : status;
    //status = (EXIT_FAILURE == GetAltitudeOnWgs84(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetGpsTime(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetTime(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetEpochTime(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetTimeAccuracy(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetDate(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetDop(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetHorizontalSpeed(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetVerticalSpeed(positionSampleRef)) ? EXIT_FAILURE : status;
    status = (EXIT_FAILURE == GetDirection(positionSampleRef)) ? EXIT_FAILURE : status;
    return status;
}


//-------------------------------------------------------------------------------------------------
/**
 * Function to do first position fix.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//-------------------------------------------------------------------------------------------------
static int DoPosFix
(
    uint32_t fixVal          ///< [IN] Position fix time in seconds
)
{
    uint32_t count = 0;
    le_result_t result = le_gnss_Start();

    if (result == LE_NOT_PERMITTED)
    {
        printf("GNSS was not enabled. Enabling it\n");
        if (le_gnss_Enable() != LE_OK)
        {
            fprintf(stderr, "Failed to enable GNSS. Try rebooting device. Exiting\n");
            return EXIT_FAILURE;
        }

        // Now start GNSS device
        if(le_gnss_Start() != LE_OK)
        {
            fprintf(stderr, "Failed to start GNSS. Try rebooting device. Exiting\n");
            return EXIT_FAILURE;
        }
    }
    else if(result == LE_FAULT)
    {
        fprintf(stderr, "Failed to start GNSS. Try rebooting device. Exiting\n");
        return EXIT_FAILURE;
    }

    result = LE_BUSY;

    while ((result == LE_BUSY) && (count < fixVal))
    {
        // Get TTFF
        uint32_t ttff;
        result = le_gnss_GetTtff(&ttff);

        if (result == LE_OK)
        {
            printf("TTFF start = %d msec\n", ttff);
            return EXIT_SUCCESS;
        }
        else if (result == LE_BUSY)
        {
            count++;
            printf("TTFF not calculated (Position not fixed)\n");
            le_thread_Sleep(1);
        }
        else
        {
            printf("Failed! See log for details\n");
            return EXIT_FAILURE;
        }
    }

    return EXIT_FAILURE;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Position Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PositionHandlerFunction
(
    le_gnss_SampleRef_t positionSampleRef,    ///< [IN] Position sample reference
    void* contextPtr                          ///< [IN] The context pointer
)
{

    if (strcmp(ParamsName, "watch") == 0)
    {
        printf("\n******** Detailed Location Information Report ********\n");
        if (FirstWatchReportTime == -1)
        {
            time(&FirstWatchReportTime);
        }
        le_mutex_Lock(gnssMutexRef);
        GetPosInfo(positionSampleRef);
        GetSatelliteStatus(positionSampleRef);
        le_gnss_FixState_t state;
        le_result_t result = le_gnss_GetPositionState( positionSampleRef, &state);
        if ((result == LE_OK) && (state == LE_GNSS_STATE_FIX_2D || state == LE_GNSS_STATE_FIX_3D))
        {
            GetSatelliteInfoEx(positionSampleRef);
            GetNmeaSentences();
            GetGnssMeasurementInfo(positionSampleRef);
            GetVRPBasedLocation(positionSampleRef);
            GetVRPBasedVelocityInfo(positionSampleRef);
            GetAltitudeMeanSeaLevel(positionSampleRef);
            GetCapabilities();
            GetSvData(positionSampleRef);
            GetTechInformation(positionSampleRef);
            GetReliabilityInfo(positionSampleRef);
            GetRealTimeInfo(positionSampleRef);
            GetSbasType(positionSampleRef);
            GetAzimuthDevInfo(positionSampleRef);
            printf("\n***********************************\n");
            GetEngineOutputParams(positionSampleRef);
            printf("***********************************\n");
        }
        printf("\n************************ End *************************\n");
        // Release provided Position sample reference
        le_gnss_ReleaseSampleRef(positionSampleRef);
        le_mutex_Unlock(gnssMutexRef);

        time(&FinishTime);
        int arrvFirstEventIn = (int) difftime(FirstWatchReportTime, StartTime);
        int acqRateInSec = (int) AcqRate/1000;
        int earlyArrvOfFirstReport = (acqRateInSec - arrvFirstEventIn) > 0 ? (acqRateInSec - arrvFirstEventIn) : 0;
        int residualWchTimeSec = acqRateInSec > 1 ? WatchPeriod % acqRateInSec : 0;
        int watchTimeSec = (int) difftime(FinishTime, StartTime);
        LE_DEBUG("Watching for: %ds, arrvFirstEventIn: %ds so earlyArrvOfFirstReport: %ds, "
                "acqRateInSec: %ds, residualWchTimeSec: %ds", watchTimeSec, arrvFirstEventIn,
                earlyArrvOfFirstReport, acqRateInSec, residualWchTimeSec);
        if (residualWchTimeSec > 0 && (earlyArrvOfFirstReport+residualWchTimeSec) > acqRateInSec)
        {
            earlyArrvOfFirstReport = (earlyArrvOfFirstReport+residualWchTimeSec) - acqRateInSec;
            residualWchTimeSec = 0;
        }
        if (watchTimeSec >= (WatchPeriod - earlyArrvOfFirstReport - residualWchTimeSec))
        {
            if (PositionHandlerRef != NULL)
            {
                le_gnss_RemovePositionHandler(PositionHandlerRef);
            }
            if (earlyArrvOfFirstReport+residualWchTimeSec > 0)
            {
                le_thread_Sleep(earlyArrvOfFirstReport+residualWchTimeSec);
            }
            PositionHandlerRef = NULL;
            GnssMainFunction();
        }
    }
    else
    {
        int status = EXIT_FAILURE;

        le_mutex_Lock(gnssMutexRef);

        if (strcmp(ParamsName, "posState") == 0)
        {
            status = GetPosState(positionSampleRef);
        }
        else if (strcmp(ParamsName, "loc2d") == 0)
        {
            status = Get2Dlocation(positionSampleRef);
        }
        else if (strcmp(ParamsName, "vrpLLA") == 0)
        {
            status = GetVRPBasedLocation(positionSampleRef);
        }
        else if (strcmp(ParamsName, "vrpVel") == 0)
        {
            status = GetVRPBasedVelocityInfo(positionSampleRef);
        }
        else if (strcmp(ParamsName, "svData") == 0)
        {
            status = GetSvData(positionSampleRef);
        }
        else if (strcmp(ParamsName, "sbasType") == 0)
        {
            status = GetSbasType(positionSampleRef);
        }
        else if (strcmp(ParamsName, "techInfo") == 0)
        {
            status = GetTechInformation(positionSampleRef);
        }
        else if (strcmp(ParamsName, "validityInfo") == 0)
        {
            status = GetValidityInfo(positionSampleRef);
        }
        else if (strcmp(ParamsName, "engParams") == 0)
        {
            status = GetEngineOutputParams(positionSampleRef);
        }
        else if (strcmp(ParamsName, "reliablityInfo") == 0)
        {
            status = GetReliabilityInfo(positionSampleRef);
        }
        else if (strcmp(ParamsName, "azimuthDevInfo") == 0)
        {
            status = GetAzimuthDevInfo(positionSampleRef);
        }
        else if (strcmp(ParamsName, "realTimeInfo") == 0)
        {
            status = GetRealTimeInfo(positionSampleRef);
        }
        else if (strcmp(ParamsName, "alt") == 0)
        {
            status = GetAltitude(positionSampleRef);
        }
        /*else if (0 == strcmp(ParamsName, "altOnWgs84"))
        {
            status = GetAltitudeOnWgs84(positionSampleRef);
        }*/
        else if (strcmp(ParamsName, "loc3d") == 0)
        {
            status = EXIT_SUCCESS;
            status = (Get2Dlocation(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
            status = (GetAltitude(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
        }
        else if (strcmp(ParamsName, "gpsTime") == 0)
        {
            status = GetGpsTime(positionSampleRef);
        }
        else if (strcmp(ParamsName, "time") == 0)
        {
            status = GetTime(positionSampleRef);
        }
        else if (strcmp(ParamsName, "epochTime") == 0)
        {
            status = GetEpochTime(positionSampleRef);
        }
        else if (strcmp(ParamsName, "timeAcc") == 0)
        {
            status = GetTimeAccuracy(positionSampleRef);
        }
        else if (strcmp(ParamsName, "date") == 0)
        {
            status = GetDate(positionSampleRef);
        }
        else if (strcmp(ParamsName, "hSpeed") == 0)
        {
            status = GetHorizontalSpeed(positionSampleRef);
        }
        else if (strcmp(ParamsName, "vSpeed") == 0)
        {
            status = GetVerticalSpeed(positionSampleRef);
        }
        else if (strcmp(ParamsName, "motion") == 0)
        {
            status = EXIT_SUCCESS;
            status = (GetHorizontalSpeed(positionSampleRef) == EXIT_FAILURE) ?
                                                                             EXIT_FAILURE : status;
            status = (GetVerticalSpeed(positionSampleRef) == EXIT_FAILURE) ? EXIT_FAILURE : status;
        }
        else if (strcmp(ParamsName, "direction") == 0)
        {
            status = GetDirection(positionSampleRef);
        }
        else if (strcmp(ParamsName, "satInfo") == 0)
        {
            status = GetSatelliteInfo(positionSampleRef);
        }
        else if (strcmp(ParamsName, "satInfoEx") == 0)
        {
            status = GetSatelliteInfoEx(positionSampleRef);
        }
        else if (strcmp(ParamsName, "satStat") == 0)
        {
            status = GetSatelliteStatus(positionSampleRef);
        }
        else if (strcmp(ParamsName, "dop") == 0)
        {
            status = GetDop(positionSampleRef);
        }
        else if (strcmp(ParamsName, "posInfo") == 0)
        {
            status = GetPosInfo(positionSampleRef);
        }

        else if (strcmp(ParamsName, "GpsLeapSeconds") == 0)
        {
            status = GetGpsLeapSeconds(positionSampleRef);
        }
        else if (strcmp(ParamsName, "magDev") == 0)
        {
            status = GetMagneticDeviation(positionSampleRef);
        }
        else if (strcmp(ParamsName, "elliUnc") == 0)
        {
            status = GetEllipticalUncertainty(positionSampleRef);
        }
        else if (strcmp(ParamsName, "conformIndex") == 0)
        {
            status = GetComformingIndex(positionSampleRef);
        }
        else if (strcmp(ParamsName, "calibData") == 0)
        {
            status = GetCablibrationConfData(positionSampleRef);
        }
        else if (strcmp(ParamsName, "drSolutionStatus") == 0)
        {
            status = GetDrSolutionStatus(positionSampleRef);
        }
        else if (strcmp(ParamsName, "bodyFrameData") == 0)
        {
            status = GetKinematicsData(positionSampleRef);
        }
        else if (strcmp(ParamsName, "measInfo") == 0)
        {
            status = GetGnssMeasurementInfo(positionSampleRef);
        }
        else if (strcmp(ParamsName, "reportStatus") == 0)
        {
            status = GetReportStatus(positionSampleRef);
        }
        else if (strcmp(ParamsName, "altMSeaLevel") == 0)
        {
            status = GetAltitudeMeanSeaLevel(positionSampleRef);
        }
        else if (strcmp(ParamsName, "svIds") == 0)
        {
            status = GetSVIds(positionSampleRef);
        }
        else if (strcmp(ParamsName, "gnssData") == 0)
        {
            status = GetGnssData(positionSampleRef);
        }
        else if (strcmp(ParamsName, "gPTPTime") == 0)
        {
            status = GetGptpTimeInformation(positionSampleRef);
        }
        else if (0 == strcmp(ParamsName, "LeapSecondsUnc"))
        {
            status = GetLeapSecondsUnc(positionSampleRef);
        }

        le_gnss_ReleaseSampleRef(positionSampleRef);

        le_mutex_Unlock(gnssMutexRef);

        ExitApp = true;
        if (PositionHandlerRef != NULL)
        {
            le_gnss_RemovePositionHandler(PositionHandlerRef);
        }

        PositionHandlerRef = NULL;

        GnssMainFunction();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for location capability notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CapabilityHandlerFunction
(
    taf_locGnss_LocCapabilityType_t locCapability,
    void* contextPtr
)
{
    DisplayCapabilities(locCapability);

    time(&FinishTime);

    if ((int) difftime(FinishTime, StartTime) >= WatchPeriod)
    {
        if (CapabilityHandlerRef != NULL)
        {
            le_gnss_RemoveCapabilityChangeHandler(CapabilityHandlerRef);
        }

        CapabilityHandlerRef =  NULL;

        GnssMainFunction();
    }
}

void DisplayNmea(const char nmeaMask[LE_GNSS_NMEA_STRING_MAX]) {
    printf("NMEA string : %s",nmeaMask);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for NMEA notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NmeaHandlerFunction
(
    uint64_t timestamp,
    const char nmeaInfo[LE_GNSS_NMEA_STRING_MAX],
    void* contextPtr
)
{
    printf("\n************* NMEA Information ***************\n");
    printf("Timestamp   : %"PRIu64" \n", timestamp);
    DisplayNmea(nmeaInfo);
    printf("**********************************************\n");

    time(&FinishTime);

    if ((int) difftime(FinishTime, StartTime) >= WatchPeriod)
    {
        if (NmeaHandlerRef != NULL)
        {
            le_gnss_RemoveNmeaHandler(NmeaHandlerRef);
        }

        NmeaHandlerRef = NULL;

        GnssMainFunction();
    }
}

static void MeasurementHandlerFunction
(
    taf_locGnss_MeasSampleRef_t measSampleRef,    ///< [IN] Position sample reference
    void* contextPtr                                  ///< [IN] The context pointer
)
{
    printf("\n******** Measurement Location Information Report ********\n");

    bool val;
    uint32_t clockValidityMask;
    size_t gnssMeasLen = TAF_LOCGNSS_MEASUREMENT_INFO_MAX;

    le_gnss_ClockData_t *clockData;
    le_mem_PoolRef_t clockDataPool = NULL;
    clockDataPool = le_mem_CreatePool("clockDataPool", sizeof(le_gnss_ClockData_t));
    clockData = (le_gnss_ClockData_t*) le_mem_ForceAlloc(clockDataPool);

    le_gnss_MeasurementsData_t measData[TAF_LOCGNSS_MEASUREMENT_INFO_MAX];
    uint32_t measDataMask[TAF_LOCGNSS_MEASUREMENT_INFO_MAX];

    le_result_t result;

    result = le_gnss_GetIsNHz(measSampleRef, &val);

    if(result == LE_OK)
    {
        printf("isNHz: %d \n",(int)val);
    }

    result = le_gnss_GetClockValidityMask(measSampleRef, &clockValidityMask);

    if(result == LE_OK)
    {
        printf("clockValidityMask: %" PRIu32 " \n",clockValidityMask);
    }

    result = le_gnss_GetClockData(measSampleRef, clockData);
    if(result == LE_OK)
    {
        printf("clockData->leapSecond:  %" PRIi16 "\n", clockData->leapSecond);
        printf("clockData->timeNs:  %" PRIi64 "\n", clockData->timeNs);
        printf("clockData->timeUncertaintyNs:  %f \n", clockData->timeUncertaintyNs);
        printf("clockData->fullBiasNs:  %" PRIi64 "\n", clockData->fullBiasNs);
        printf("clockData->biasNs:  %f\n", clockData->biasNs);
        printf("clockData->biasUncertaintyNs:  %f\n", clockData->biasUncertaintyNs);
        printf("clockData->driftNsps:  %f\n", clockData->driftNsps);
        printf("clockData->driftUncertaintyNsps:  %f\n", clockData->driftUncertaintyNsps);
        printf("clockData->hwClockDiscontinuityCount:  %" PRIu32 "\n", clockData->hwClockDiscontinuityCount);
        printf("clockData->elapsedRealTime:  %" PRIu64 "\n", clockData->elapsedRealTime);
        printf("clockData->elapsedRealTimeUnc:  %" PRIu64 "\n", clockData->elapsedRealTimeUnc);
        printf("clockData->elapsedgPTPTime:  %" PRIu64 "\n", clockData->elapsedgPTPTime);
        printf("clockData->elapsedgPTPTimeUnc:  %" PRIu64 "\n", clockData->elapsedgPTPTimeUnc);
    }

    result = le_gnss_GetMeasurementsData(measSampleRef, measData, &gnssMeasLen);
    if(result == LE_OK)
    {
        printf("gnssMeasLen: %d\n",(int)gnssMeasLen);
        for(size_t i=0;i<gnssMeasLen;i++){
            printf("svId: %" PRIi16 "\n",measData[i].svId);
            printf("svType: %" PRIu32 "\n", measData[i].svType);
            printf("timeOffsetNs: %f\n", measData[i].timeOffsetNs);
            printf("stateMask: %" PRIu32 "\n", measData[i].stateMask);
            printf("receivedSvTimeNs: %" PRIi64 "\n", measData[i].receivedSvTimeNs);
            printf("receivedSvTimeSubNs: %f\n", measData[i].receivedSvTimeSubNs);
            printf("receivedSvTimeUncertaintyNs: %" PRIi64 "\n", measData[i].receivedSvTimeUncertaintyNs);
            printf("carrierToNoiseDbHz: %f\n", measData[i].carrierToNoiseDbHz);
            printf("pseudorangeRateMps: %f\n", measData[i].pseudorangeRateMps);
            printf("pseudorangeRateUncertaintyMps: %f\n", measData[i].pseudorangeRateUncertaintyMps);
            printf("adrStateMask: %" PRIu32 "\n", measData[i].adrStateMask);
            printf("adrMeters: %f\n", measData[i].adrMeters);
            printf("adrUncertaintyMeters: %f\n", measData[i].adrUncertaintyMeters);
            printf("carrierFrequencyHz: %f\n", measData[i].carrierFrequencyHz);
            printf("carrierCycles: %" PRIi64 "\n", measData[i].carrierCycles);
            printf("carrierPhase: %f\n", measData[i].carrierPhase);
            printf("carrierPhaseUncertainty: %f\n", measData[i].carrierPhaseUncertainty);
            printf("multipathIndicator: %d\n", (int)measData[i].multipathIndicator);
            printf("signalToNoiseRatioDb: %f\n", measData[i].signalToNoiseRatioDb);
            printf("agcLevelDb: %f\n", measData[i].agcLevelDb);
            printf("gnssSignalType: %" PRIu32 "\n", measData[i].gnssSignalType);
            printf("basebandCarrierToNoise: %f\n", measData[i].basebandCarrierToNoise);
            printf("fullInterSignalBias: %f\n", measData[i].fullInterSignalBias);
            printf("fullInterSignalBiasUncertainty: %f\n", measData[i].fullInterSignalBiasUncertainty);
        }
    }

    result = le_gnss_GetMeasDataValidityMask(measSampleRef, measDataMask, &gnssMeasLen);
    if(result == LE_OK)
    {
        LE_TEST_INFO("gnssMeasLen: %d\n",(int)gnssMeasLen);
        for(size_t i=0;i<gnssMeasLen;i++){
            printf("measDataMask[%zu]: %" PRIu32 "\n",i,measDataMask[i]);
        }
    }

    le_gnss_ReleaseMeasSampleRef(measSampleRef);

    time(&FinishTime);

    if ((int) difftime(FinishTime, StartTime) >= WatchPeriod)
    {
        if (MeasHandlerRef != NULL)
        {
            le_gnss_RemoveMeasurementHandler(MeasHandlerRef);
        }

        MeasHandlerRef = NULL;

        GnssMainFunction();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to enable gnss and monitor its information.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//--------------------------------------------------------------------------------------------------
static int WatchGnssInfo
(
    uint32_t watchPeriod          ///< [IN] Watch period in seconds
)
{
    WatchPeriod = watchPeriod;

    FirstWatchReportTime = -1;

    time(&StartTime);

    le_gnss_GetAcquisitionRate(&AcqRate);
    LE_DEBUG("Watch Gnss info for %ds at interval of (AcqRate) %dms", WatchPeriod, AcqRate);

    // Add Position Handler
    PositionHandlerRef = le_gnss_AddPositionHandler(PositionHandlerFunction, NULL);
    LE_ASSERT((PositionHandlerRef != NULL));

    printf("Watch positioning data for %ds\n", watchPeriod);

    return EXIT_SUCCESS;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to watch location capability information.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//--------------------------------------------------------------------------------------------------
static int WatchGnssCapInfo
(
    uint32_t watchPeriod          ///< [IN] Watch period in seconds
)
{

    WatchPeriod = watchPeriod;

    time(&StartTime);

    CapabilityHandlerRef = le_gnss_AddCapabilityChangeHandler(CapabilityHandlerFunction, NULL);
    LE_ASSERT(CapabilityHandlerRef != NULL);

    printf("Watch GNSS capabilities data for %ds\n", watchPeriod);

    return EXIT_SUCCESS;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to watch NMEA information.
 *
 * @return
 *     - EXIT_SUCCESS on success.
 *     - EXIT_FAILURE on failure.
 */
//--------------------------------------------------------------------------------------------------
static int WatchGnssNmeaInfo
(
    uint32_t watchPeriod          ///< [IN] Watch period in seconds
)
{
    WatchPeriod = watchPeriod;

    time(&StartTime);

    NmeaHandlerRef = le_gnss_AddNmeaHandler(NmeaHandlerFunction, NULL);
    LE_ASSERT(NmeaHandlerRef != NULL);

    printf("Watch NMEA data for %ds\n", watchPeriod);

    return EXIT_SUCCESS;
}

static int WatchMeasurementInfo
(
    uint32_t watchPeriod          ///< [IN] Watch period in seconds
)
{
    WatchPeriod = watchPeriod;

    time(&StartTime);

    MeasHandlerRef = le_gnss_AddMeasurementHandler(MeasurementHandlerFunction, NULL);
    LE_ASSERT(MeasHandlerRef != NULL);

    printf("Watch Measurement data for %ds\n", watchPeriod);

    return EXIT_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function prints the GNSS device status.
 */
//-------------------------------------------------------------------------------------------------
static int GetGnssDeviceStatus
(
void
)
{
    le_gnss_State_t state;
    const char *status;

    state = le_gnss_GetState();

    switch (state)
    {
        case LE_GNSS_STATE_UNINITIALIZED:
            status = "not initialized";
            break;
        case LE_GNSS_STATE_READY:
            status = "ready";
            break;
        case LE_GNSS_STATE_ACTIVE:
            status = "active";
            break;
        case LE_GNSS_STATE_DISABLED:
            status = "disabled";
            break;
        default:
            status = "unknown";
            break;
    }

    fprintf(stdout, "%s\n", status);

    return 0;
};

//-------------------------------------------------------------------------------------------------
/**
 * This function gets different gnss parameters.
 */
//-------------------------------------------------------------------------------------------------
static void GetGnssParams
(
    const char *params      ///< [IN] gnss parameters
)
{
    le_gnss_State_t state = le_gnss_GetState();

    if (0 == strcmp(params, "ttff"))
    {
        GetTtff(state);
    }
    else if (0 == strcmp(params, "acqRate"))
    {
        GetAcquisitionRate();
    }
    else if (0 == strcmp(params, "LeapSeconds"))
    {
        GetLeapSeconds();
    }
    /*else if (0 == strcmp(params, "agpsMode"))
    {
        GetAgpsMode();
    }*/
    else if (0 == strcmp(params, "constellation"))
    {
        GetConstellation();
    }
    else if (0 == strcmp(params, "secondBandConst"))
    {
        RequestSecondaryBandConstellations();
    }
    else if (0 == strcmp(params, "robustloc"))
    {
        RobustLocationInformation();
    }
    #if 0
    else if (0 == strcmp(params, "constArea"))
    {
        GetConstellationArea();
    }
    #endif
    else if (0 == strcmp(params, "nmeaSentences"))
    {
        GetNmeaSentences();
    }
    else if (0 == strcmp(params, "minElevation"))
    {
        GetMinElevation();
    }
    else if (0 == strcmp(params, "minGpsWeek"))
    {
        GetMinGpsWeek();
    }
    else if (0 == strcmp(params, "locCap"))
    {
        GetCapabilities();
    }
    else if (0 == strcmp(params, "xtraStatus"))
    {
        GetXtraStatus();
    }
    else if ((0 == strcmp(params, "posState"))    ||
             (0 == strcmp(params, "loc2d"))       ||
             (0 == strcmp(params, "alt"))         ||
             (0 == strcmp(params, "loc3d"))       ||
             (0 == strcmp(params, "gpsTime"))     ||
             (0 == strcmp(params, "time"))        ||
             (0 == strcmp(params, "epochTime"))   ||
             (0 == strcmp(params, "timeAcc"))     ||
             (0 == strcmp(params, "LeapSeconds")) ||
             (0 == strcmp(params, "date"))        ||
             (0 == strcmp(params, "hSpeed"))      ||
             (0 == strcmp(params, "vSpeed"))      ||
             (0 == strcmp(params, "motion"))      ||
             (0 == strcmp(params, "direction"))   ||
             (0 == strcmp(params, "satInfo"))     ||
             (0 == strcmp(params, "satInfoEx"))   ||
             (0 == strcmp(params, "satStat"))     ||
             (0 == strcmp(params, "dop"))         ||
             (0 == strcmp(params, "magDev"))      ||
             (0 == strcmp(params, "elliUnc"))     ||
             (0 == strcmp(params,"GpsLeapSeconds"))||
             (0 == strcmp(params, "posInfo"))     ||
             (0 == strcmp(params, "conformIndex"))||
             (0 == strcmp(params, "calibData"))||
             (0 == strcmp(params, "bodyFrameData"))||
             (0 == strcmp(params, "vrpLLA"))||
             (0 == strcmp(params, "vrpVel"))||
             (0 == strcmp(params, "svData"))||
             (0 == strcmp(params, "sbasType"))||
             (0 == strcmp(params, "techInfo"))||
             (0 == strcmp(params, "validityInfo"))||
             (0 == strcmp(params, "engParams"))||
             (0 == strcmp(params, "reliablityInfo"))||
             (0 == strcmp(params, "azimuthDevInfo"))||
             (0 == strcmp(params, "realTimeInfo"))||
             (0 == strcmp(params, "measInfo"))||
             (0 == strcmp(params, "altMSeaLevel"))||
             (0 == strcmp(params, "svIds"))||
             (0 == strcmp(params, "reportStatus"))||
             (0 == strcmp(params, "gnssData"))||
             (0 == strcmp(params,"gPTPTime"))||
             (0 == strcmp(params,"drSolutionStatus"))||
             (0 == strcmp(params,"LeapSecondsUnc")))
    {
        if (LE_GNSS_STATE_ACTIVE != state)
        {
            printf("GNSS is not in active state!\n");
            return;
        }
        ExitApp = false;

        // Copy the param
        le_utf8_Copy(ParamsName, params, sizeof(ParamsName), NULL);

        PositionHandlerRef = le_gnss_AddPositionHandler(PositionHandlerFunction, NULL);
        LE_ASSERT((PositionHandlerRef != NULL));
    }
    else if (strcmp(params, "status") == 0)
    {
        GetGnssDeviceStatus();
    }
    else
    {
        printf("Bad parameter: %s\n", params);
    }

}


//-------------------------------------------------------------------------------------------------
/**
 * This function sets different gnss parameters .
 */
//-------------------------------------------------------------------------------------------------
static int SetGnssParams
(
    const char * argNamePtr,    ///< [IN] gnss parameters
    const char * argValPtr,     ///< [IN] gnss parameters
    const char * arg2ValPtr     ///< [IN] gnss parameters
)
{
    int status = EXIT_FAILURE;

    if (strcmp(argNamePtr, "constellation") == 0)
    {
        status = SetConstellation(argValPtr);
    }
    #if 0
    else if (strcmp(argNamePtr, "constArea") == 0)
    {
        if (NULL == arg2ValPtr)
        {
            LE_ERROR("arg2ValPtr is NULL");
            return EXIT_FAILURE;
        }
        status = SetConstellationArea(argValPtr, arg2ValPtr);
    }
    #endif
    else if (strcmp(argNamePtr, "acqRate") == 0)
    {
        status = SetAcquisitionRate(argValPtr);
    }
    /*else if (strcmp(argNamePtr, "agpsMode") == 0)
    {
        status = SetAgpsMode(argValPtr);
    }*/
    else if (strcmp(argNamePtr, "nmeaSentences") == 0)
    {
        status = SetNmeaSentences(argValPtr);
    }
    else if (strcmp(argNamePtr, "nmeaConfig") == 0)
    {
        const char* arg3ValPtr = TokenArray[4];
        status = SetNmeaConfiguration(argValPtr, arg2ValPtr, arg3ValPtr);
    }
    else if (0 == strcmp(argNamePtr, "minElevation"))
    {
        status = SetMinElevation(argValPtr);
    }
    else if (0 == strcmp(argNamePtr, "minGpsWeek"))
    {
        status = SetMinGpsWeek(argValPtr);
    }
    else if (0 == strcmp(argNamePtr, "startMode"))
    {
       status = StartMode(argValPtr);
    }
    else if (0 == strcmp(argNamePtr, "configEng"))
    {
        if (NULL == arg2ValPtr)
        {
            printf("arg2ValPtr is NULL");
            return EXIT_FAILURE;
        }
        status = ConfigureEngineState(argValPtr, arg2ValPtr);
    }
    else if (0 == strcmp(argNamePtr, "robustloc"))
    {
        if (NULL == arg2ValPtr)
        {
            printf("arg2ValPtr is NULL");
            return EXIT_FAILURE;
        }
        status = ConfigureRobustLocation(argValPtr,arg2ValPtr);
    }
    else if (0 == strcmp(argNamePtr, "secondBandConst"))
    {
        status = ConfigureSecondaryBandConstellations(argValPtr);
    }
    else
    {
        printf("Bad parameter request: %s\n", argNamePtr);
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * Verify if enough parameter passed into command.
 * If not, output error message and terminate the program.
 */
//--------------------------------------------------------------------------------------------------
void CheckEnoughParams
(
    size_t requiredParam,      ///< [IN] Required parameters for the command
    size_t numArgs,            ///< [IN] Number of arguments passed into the command line
    const char * errorMsgPtr   ///< [IN] Error message to output if not enough parameters
)
{
    if ( (requiredParam + 1) <= numArgs)
    {
        return;
    }
    else
    {
        printf("%s\nTry '%s help'\n", errorMsgPtr, le_arg_GetProgramName());
        return;
    }
}

void GnssMainFunction
(
    void
)
{
    le_result_t result = LE_FAULT;
    char inputStr[MAX_NUMBER_OF_INPUT * MAX_LEN_OF_EACH_INPUT];
    int tokenNum = 0; // Index to token list.

    do
    {
        memset(inputStr, '\0', MAX_NUMBER_OF_INPUT * MAX_LEN_OF_EACH_INPUT);
        for (int i = 0; i < MAX_NUMBER_OF_INPUT; i++)
        {
            memset(TokenArray[i], '\0', MAX_LEN_OF_EACH_INPUT);
        }

        le_gnss_State_t state = le_gnss_GetState();
        if (TAF_LOCGNSS_STATE_ACTIVE == state)
        {
            printf("\033[1;32mgnss> \033[0m"); // Color GREEN
        }
        else
        {
            printf("\033[1;35mgnss> \033[0m"); // Color MAGENTA
        }

        tokenNum = 0;
        char *p = fgets(inputStr, sizeof(inputStr), stdin);

        if (p != NULL)
        {
            LE_INFO("Input string is: %s, string length: %d", inputStr, (int) strlen(inputStr));
            char *context = NULL;
            char *token = strtok_r(inputStr, " ", &context);
            while (token != NULL)
            {
                if (tokenNum < MAX_NUMBER_OF_INPUT)
                {
                    le_utf8_Copy(TokenArray[tokenNum], token, sizeof(TokenArray[tokenNum]), NULL);
                    tokenNum++;
                    token = strtok_r(NULL, " ", &context);
                }
                else
                {
                    break;
                }
            }

            if (tokenNum < 1)
            {
                return;
            }
            int nullAt = strlen(TokenArray[tokenNum - 1]);
            TokenArray[tokenNum - 1][nullAt - 1] = '\0';
        }
        else
        {
            return;
        }

        // Process the command
        ExitApp = true;
        if (tokenNum < 1)
        {
            // No argument specified.
            continue;
        }

        const char *commandPtr = TokenArray[0];
        if (NULL == commandPtr)
        {
            LE_ERROR("commandPtr is NULL");
        }

        size_t numArgs = tokenNum;

        if (strcmp(commandPtr, "h") == 0 || strcmp(commandPtr, "?") == 0 ||
            strcmp(commandPtr, "help") == 0)
        {
            PrintGnssHelp();
        }
        else if (strcmp(commandPtr, "start") == 0)
        {
            Start();
        }
        else if (0 == strcmp(commandPtr, "startType"))
        {
            const char *startTypePtr = TokenArray[1];
            if (startTypePtr == NULL)
            {
                printf("start type is NULL.\n");
                continue;
            }

            StartEngineType(startTypePtr);
        }
        else if (0 == strcmp(commandPtr, "ConfigLevArm"))
        {
            const char *forwOffsetPtr = TokenArray[1];
            if (forwOffsetPtr == NULL)
            {
                printf("forwOffsetPtr is NULL.\n");
                continue;
            }

            const char *sideOffsetPtr = TokenArray[2];
            if (sideOffsetPtr == NULL)
            {
                printf("sideOffsetPtr is NULL.\n");
                continue;
            }

            const char *upOffsePtr = TokenArray[3];
            if (upOffsePtr == NULL)
            {
                printf("upOffsePtr is NULL.\n");
                continue;
            }

            const char *levArmTypePtr = TokenArray[4];
            if (levArmTypePtr == NULL)
            {
                printf("levArmTypePtr is NULL.\n");
                continue;
            }

            le_gnss_LeverArmParams_t * leverArmParamsPtr;
            char *endPtr;
            double forwardOffsetMeters;
            double sidewaysOffsetMeters;
            double upOffsetMeters;
            uint32_t levArmType;
            forwardOffsetMeters = strtod(forwOffsetPtr, &endPtr);
            if (endPtr[0] != '\0')
            {
                printf("Bad forwOffsetPtr: %s\n", forwOffsetPtr);
                continue;
            }

            sidewaysOffsetMeters = strtod(sideOffsetPtr, &endPtr);
            if (endPtr[0] != '\0')
            {
                printf("Bad sideOffsetPtr: %s\n", sideOffsetPtr);
                continue;
            }

            upOffsetMeters = strtod(upOffsePtr, &endPtr);
            if (endPtr[0] != '\0')
            {
                printf("Bad upOffsePtr: %s\n", upOffsePtr);
                continue;
            }

            levArmType = strtod(levArmTypePtr, &endPtr);
            if (endPtr[0] != '\0')
            {
                printf("Bad levArmTypePtr: %s\n", levArmTypePtr);
                continue;
            }

            le_mem_PoolRef_t LevArmFramePool = NULL;
            LevArmFramePool
                = le_mem_CreatePool("LevArmFramePool", sizeof(le_gnss_LeverArmParams_t));
            leverArmParamsPtr
                = (le_gnss_LeverArmParams_t*) le_mem_ForceAlloc(LevArmFramePool);
            leverArmParamsPtr->forwardOffsetMeters = forwardOffsetMeters;
            leverArmParamsPtr->sidewaysOffsetMeters = sidewaysOffsetMeters;
            leverArmParamsPtr->upOffsetMeters = upOffsetMeters;
            leverArmParamsPtr->levArmType = levArmType;
            ConfigureLevArm(leverArmParamsPtr);
        }
        else if (0 == strcmp(commandPtr, "ConfigDR"))
        {
            const char *rollOffsetPtr = TokenArray[1];
            if (rollOffsetPtr == NULL)
            {
                printf("rollOffsetPtr is NULL.\n");
                continue;
            }

            const char *yawOffsetPtr = TokenArray[2];
            if (yawOffsetPtr == NULL)
            {
                printf("yawOffsetPtr is NULL.\n");
                continue;
            }

            const char *pitchOffsetPtr = TokenArray[3];
            if (pitchOffsetPtr == NULL)
            {
                printf("pitchOffsetPtr is NULL.\n");
                continue;
            }

            const char *offsetUncPtr = TokenArray[4];
            if (offsetUncPtr == NULL)
            {
                printf("offsetUncPtr is NULL.\n");
                continue;
            }

            const char *speedFactorPtr = TokenArray[5];
            if (speedFactorPtr == NULL)
            {
                printf("speedFactorPtr is NULL.\n");
                continue;
            }

            const char *speedFactorUncPtr = TokenArray[6];
            if (speedFactorUncPtr == NULL)
            {
                printf("speedFactorUncPtr is NULL.\n");
                continue;
            }

            const char *gyroFactorPtr = TokenArray[7];
            if (gyroFactorPtr == NULL)
            {
                printf("gyroFactorPtr is NULL.\n");
                continue;
            }

            const char *gyroFactorUncPtr = TokenArray[8];
            if (gyroFactorUncPtr == NULL)
            {
                printf("gyroFactorUncPtr is NULL.\n");
                continue;
            }

            le_gnss_DrParams_t *DrParamsPtr;
            char *endPtr;
            double rollOffset;
            double yawOffset;
            double pitchOffset;
            double offsetUnc;
            double speedFactor;
            double speedFactorUnc;
            double gyroFactor;
            double gyroFactorUnc;
            rollOffset = strtod(rollOffsetPtr, &endPtr);
            if (endPtr[0] != '\0')
            {
                printf("Bad rollOffsetPtr: %s\n", rollOffsetPtr);
                continue;
            }

            yawOffset = strtod(yawOffsetPtr, &endPtr);
            if (endPtr[0] != '\0')
            {
                printf("Bad yawOffsetPtr: %s\n", yawOffsetPtr);
                continue;
            }

            pitchOffset = strtod(pitchOffsetPtr, &endPtr);
            if (endPtr[0] != '\0')
            {
                printf("Bad pitchOffsetPtr: %s\n", pitchOffsetPtr);
                continue;
            }

            offsetUnc = strtod(offsetUncPtr, &endPtr);
            if (endPtr[0] != '\0')
            {
                printf("Bad offsetUncPtr: %s\n", offsetUncPtr);
                continue;
            }

            speedFactor = strtod(speedFactorPtr, &endPtr);
            if (endPtr[0] != '\0')
            {
                printf("Bad speedFactorPtr: %s\n", speedFactorPtr);
                continue;
            }

            speedFactorUnc = strtod(speedFactorUncPtr, &endPtr);
            if (endPtr[0] != '\0')
            {
                printf("Bad speedFactorUncPtr: %s\n", speedFactorUncPtr);
                continue;
            }

            gyroFactor = strtod(gyroFactorPtr, &endPtr);
            if (endPtr[0] != '\0')
            {
                printf("Bad gyroFactorPtr: %s\n", gyroFactorPtr);
                continue;
            }

            gyroFactorUnc = strtod(gyroFactorUncPtr, &endPtr);
            if (endPtr[0] != '\0')
            {
                printf("Bad gyroFactorUncPtr: %s\n", gyroFactorUncPtr);
                continue;
            }

            le_mem_PoolRef_t DrFramePool = NULL;
            DrFramePool = le_mem_CreatePool("DrFramePool", sizeof(le_gnss_DrParams_t));
            DrParamsPtr = (le_gnss_DrParams_t*) le_mem_ForceAlloc(DrFramePool);
            if (DrParamsPtr != NULL)
            {
                DrParamsPtr->rollOffset = rollOffset;
                DrParamsPtr->yawOffset = yawOffset;
                DrParamsPtr->pitchOffset = pitchOffset;
                DrParamsPtr->offsetUnc = offsetUnc;
                DrParamsPtr->speedFactor = speedFactor;
                DrParamsPtr->speedFactorUnc = speedFactorUnc;
                DrParamsPtr->gyroFactor = gyroFactor;
                DrParamsPtr->gyroFactorUnc = gyroFactorUnc;
                ConfigureDeadReckoning(DrParamsPtr);
            }
            else
            {
                continue;
            }
        }
        else if (0 == strcmp(commandPtr, "DRValidity"))
        {
            const char *validityMaskPtr = TokenArray[1];
            if (validityMaskPtr == NULL)
            {
                printf("DR parameters validityMaskPtr is NULL.\n");
                continue;
            }
            SetDRValidityMask(validityMaskPtr);
        }
        else if (strcmp(commandPtr, "stop") == 0)
        {
            Stop();
        }
        else if (strcmp(commandPtr, "enable") == 0)
        {
            Enable();
        }
        else if (strcmp(commandPtr, "disable") == 0)
        {
            Disable();
        }
        else if (strcmp(commandPtr, "delete") == 0)//delete DR sensor calib data-DRCalibData
        {
            const char *deleteTypePtr = TokenArray[1];
            if (deleteTypePtr == NULL)
            {
                printf("delete type is NULL.\n");
                continue;
            }
            // Following function exit on failure, so no need to check return code.
            CheckEnoughParams(1, numArgs, "delete type missing");
            Delete(deleteTypePtr);
        }
        else if (strcmp(commandPtr, "restart") == 0)
        {
            const char *restartTypePtr = TokenArray[1];
            if (restartTypePtr == NULL)
            {
                printf("Restart type is NULL.\n");
                continue;
            }

            // Following function exit on failure, so no need to check return code.
            CheckEnoughParams(1, numArgs, "Restart type missing");
            Restart(restartTypePtr);
        }
        else if (strcmp(commandPtr, "fix") == 0)
        {
            const char *fixPeriodPtr = TokenArray[1];
            uint32_t fixPeriod = DEFAULT_3D_FIX_TIME;
            // Check whether any watch period value is specified.
            if (NULL != fixPeriodPtr)
            {
                char *endPtr;
                errno = 0;
                fixPeriod = strtoul(fixPeriodPtr, &endPtr, 10);

                if (endPtr[0] != '\0' || errno != 0)
                {
                    fprintf(stderr, "Bad fix period value: %s\n", fixPeriodPtr);
                    continue;
                }
            }

            le_gnss_State_t state = le_gnss_GetState();
            if (TAF_LOCGNSS_STATE_ACTIVE != state)
            {
                printf("GNSS is not in active state!\n");
                continue;
            }

            if (fixPeriod == 0)
            {
                fixPeriod = DEFAULT_3D_FIX_TIME;
            }

            DoPosFix(fixPeriod);
        }
        else if (strcmp(commandPtr, "supportedNmeaSentences") == 0)
        {
            GetSupportedNmeaSentences();
        }
        else if (strcmp(commandPtr, "supportedConstellations") == 0)
        {
            GetSupportedConstellations();
        }
        else if (strcmp(commandPtr, "configDefSecBand") == 0)
        {
            DefaultSecondaryBandConstellations();
        }
        else if (strcmp(commandPtr, "get") == 0)
        {
            const char *paramsPtr = TokenArray[1];
            if (NULL == paramsPtr)
            {
                LE_ERROR("paramsPtr is NULL");
                continue;
            }

            CheckEnoughParams(1, numArgs, "Missing arguments");
            GetGnssParams(paramsPtr);
        }
        else if (strcmp(commandPtr, "set") == 0)
        {
            const char *argNamePtr = TokenArray[1];
            const char *argValPtr = TokenArray[2];
            const char *arg2ValPtr = TokenArray[3];
            if (NULL == argNamePtr)
            {
                LE_ERROR("argNamePtr is NULL");
                printf("argNamePtr is NULL");
                continue;
            }

            if (NULL == argValPtr)
            {
                LE_ERROR("argValPtr is NULL");
                printf("argValPtr is NULL");
                continue;
            }

            CheckEnoughParams(2, numArgs, "Missing arguments");
            SetGnssParams(argNamePtr, argValPtr, arg2ValPtr);
        }
        else if (strcmp(commandPtr, "watch") == 0)
        {
            if (LE_GNSS_STATE_ACTIVE != le_gnss_GetState())
            {
                printf("GNSS is not in active state!\n");
                continue;
            }

            const char *watchPeriodPtr = TokenArray[1];
            uint32_t watchPeriod = DEFAULT_WATCH_PERIOD;
            // Check whether any watch period value is specified.
            if (NULL != watchPeriodPtr)
            {
                char *endPtr;
                errno = 0;
                watchPeriod = strtoul(watchPeriodPtr, &endPtr, 10);

                if (endPtr[0] != '\0' || errno != 0)
                {
                    fprintf(stderr, "Bad watch period value: %s\n", watchPeriodPtr);
                    continue;
                }
                if (strcmp(watchPeriodPtr, "") == 0)
                {
                    //No input of Watch period so use default watch period.
                    watchPeriod = DEFAULT_WATCH_PERIOD;
                }
            }

            uint32_t acqRate;
            result = le_gnss_GetAcquisitionRate(&acqRate);
            if (watchPeriod == 0 || (LE_OK == result && acqRate > watchPeriod*1000))
            {
                printf("Watch positioning data for %ds\n", watchPeriod);
                le_thread_Sleep(watchPeriod);
                continue;
            }

            // Copy the command
            le_utf8_Copy(ParamsName, commandPtr, sizeof(ParamsName), NULL);
            WatchGnssInfo(watchPeriod);
            ExitApp = false;
        }
        else if (strcmp(commandPtr, "capwatch") == 0)
        {
            if (LE_GNSS_STATE_ACTIVE != le_gnss_GetState())
            {
                printf("GNSS is not in active state!\n");
                continue;
            }

            const char *capwatchPeriodPtr = TokenArray[1];
            uint32_t capwatchPeriod = DEFAULT_WATCH_PERIOD;
            // Check whether any watch period value is specified.
            if (NULL != capwatchPeriodPtr)
            {
                char *endPtr;
                errno = 0;
                capwatchPeriod = strtoul(capwatchPeriodPtr, &endPtr, 10);

                if (endPtr[0] != '\0' || errno != 0)
                {
                    fprintf(stderr, "Bad watch period value: %s\n", capwatchPeriodPtr);
                    continue;
                }
                if (strcmp(capwatchPeriodPtr, "") == 0)
                {
                    //No input of Watch period so use default watch period.
                    capwatchPeriod = DEFAULT_WATCH_PERIOD;
                }
            }

            // Copy the command
            le_utf8_Copy(ParamsName, commandPtr, sizeof(ParamsName), NULL);
            if (capwatchPeriod == 0)
            {
                printf("Watch GNSS capabilities data for %ds\n", capwatchPeriod);
                continue;
            }
            WatchGnssCapInfo(capwatchPeriod);
            ExitApp = false;
        }
        else if (strcmp(commandPtr, "nmeawatch") == 0)
        {
            if (LE_GNSS_STATE_ACTIVE != le_gnss_GetState())
            {
                printf("GNSS is not in active state!\n");
                continue;
            }

            const char *nmeawatchPeriodPtr = TokenArray[1];
            uint32_t nmeawatchPeriod = DEFAULT_WATCH_PERIOD;
            // Check whether any watch period value is specified.
            if (NULL != nmeawatchPeriodPtr)
            {
                char *endPtr;
                errno = 0;
                nmeawatchPeriod = strtoul(nmeawatchPeriodPtr, &endPtr, 10);

                if (endPtr[0] != '\0' || errno != 0)
                {
                    fprintf(stderr, "Bad watch period value: %s\n", nmeawatchPeriodPtr);
                    continue;
                }
                if (strcmp(nmeawatchPeriodPtr, "") == 0)
                {
                    //No input of Watch period so use default watch period.
                    nmeawatchPeriod = DEFAULT_WATCH_PERIOD;
                }
            }

            // Copy the command
            le_utf8_Copy(ParamsName, commandPtr, sizeof(ParamsName), NULL);
            if (nmeawatchPeriod == 0) {
                printf("Watch NMEA data for %ds\n", nmeawatchPeriod);
                continue;
            }
            WatchGnssNmeaInfo(nmeawatchPeriod);
            ExitApp = false;
        }
        else if (strcmp(commandPtr, "measwatch") == 0)
        {
            if (LE_GNSS_STATE_ACTIVE != le_gnss_GetState())
            {
                printf("GNSS is not in active state!\n");
                continue;
            }

            const char *measwatchPeriodPtr = TokenArray[1];
            uint32_t measwatchPeriod = DEFAULT_WATCH_PERIOD;
            // Check whether any watch period value is specified.
            if (NULL != measwatchPeriodPtr)
            {
                char *endPtr;
                errno = 0;
                measwatchPeriod = strtoul(measwatchPeriodPtr, &endPtr, 10);

                if (endPtr[0] != '\0' || errno != 0)
                {
                    fprintf(stderr, "Bad watch period value: %s\n", measwatchPeriodPtr);
                    continue;
                }
                if (strcmp(measwatchPeriodPtr, "") == 0)
                {
                    //No input of Watch period so use default watch period.
                    measwatchPeriod = DEFAULT_WATCH_PERIOD;
                }
            }

            // Copy the command
            le_utf8_Copy(ParamsName, commandPtr, sizeof(ParamsName), NULL);
            if (measwatchPeriod == 0) {
                printf("Watch Measurement data for %ds\n", measwatchPeriod);
                continue;
            }
            WatchMeasurementInfo(measwatchPeriod);
            ExitApp = false;
        }
        else
        {
            if (inputStr[0] != 'q' && inputStr[0] != '0')
            {
                printf("Invalid command for GNSS service. Input 'help' to check usages.\n");
            }
        }
    } while (inputStr[0] != 'q' && inputStr[0] != '0' && ExitApp);

    if (ExitApp)
    {
        if (TAF_LOCGNSS_STATE_ACTIVE == le_gnss_GetState())
        {
            result = le_gnss_Stop();
        }

        LE_INFO("result of le_gnss_Stop is %d", (int) result);
        if (PositionHandlerRef != NULL)
        {
            le_gnss_RemovePositionHandler(PositionHandlerRef);
        }

        if (CapabilityHandlerRef != NULL)
        {
            le_gnss_RemoveCapabilityChangeHandler(CapabilityHandlerRef);
        }

        if (NmeaHandlerRef != NULL)
        {
            le_gnss_RemoveNmeaHandler(NmeaHandlerRef);
        }

        exit(EXIT_SUCCESS);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Program init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    const char* gnssHelp = le_arg_GetArg(0);
    if(gnssHelp != NULL)
    {
        if ((le_arg_NumArgs() > 0) && (strcmp(gnssHelp, "help") == 0))
        {
            LE_INFO("PrintGnssHelp menu for 'gnss help' command ");
            PrintGnssHelp();
        }
    }
    else
    {
        LE_INFO("Launch Gnss tool for 'gnss' command\n");
    }

    gnssMutexRef = le_mutex_CreateNonRecursive("GnssMutex");

    GnssMainFunction();
}


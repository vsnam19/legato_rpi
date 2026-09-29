//--------------------------------------------------------------------------------------------------
/** @file supervisor/supervisor.h
 *
 * API for working with Legato supervisor.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#ifndef LEGATO_SRC_SUPERVISOR_INCLUDE_GUARD
#define LEGATO_SRC_SUPERVISOR_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Enumerates the different application start options that can be provided on the command-line.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    APP_START_AUTO,     ///< Start all apps that are marked for automatic start.
    APP_START_NONE,     ///< Don't start any apps until told to do so through the App Control API.
    APP_START_GROUP,    ///< Supervisor only starts the app with a start group number that does not
                        ///  exceed the value defined in the macro LE_CONFIG_LIMIT_APP_START_GROUP.
}
AppStartMode_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reboot the system.
 */
//--------------------------------------------------------------------------------------------------
void framework_Reboot
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Reports if the Legato framework is stopping.
 *
 * @return
 *     true if the framework is stopping or rebooting
 *     false otherwise
 */
//--------------------------------------------------------------------------------------------------
bool framework_IsStopping
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the app start mode.
 *
 * @return
 *     app start mode.
 */
//--------------------------------------------------------------------------------------------------
AppStartMode_t framework_GetAppStartMode
(
    void
);

#endif // LEGATO_SRC_SUPERVISOR_INCLUDE_GUARD

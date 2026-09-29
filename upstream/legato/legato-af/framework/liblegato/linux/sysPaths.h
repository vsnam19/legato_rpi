//--------------------------------------------------------------------------------------------------
/** @file sysPaths.h
 *
 * This file defines several common system related paths. Currently they are simply #defined as
 * strings in this header.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#ifndef LEGATO_SYSPATHS_INCLUDE_GUARD
#define LEGATO_SYSPATHS_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Absolute file system path to where systems are installed.
 */
//--------------------------------------------------------------------------------------------------
#define SYSTEM_PATH "/legato/systems"

//--------------------------------------------------------------------------------------------------
/**
 * Absolute TelAF app path.
 */
//--------------------------------------------------------------------------------------------------
#define TELAF_APP_PATH "/app"

//--------------------------------------------------------------------------------------------------
/**
 * Absolute TelAF app installation path which is used as overlay upper dir.
 */
//--------------------------------------------------------------------------------------------------
#define TELAF_APP_OVERLAYFS_UPPER_PATH TELAF_APP_PATH"/appsInstall"

//--------------------------------------------------------------------------------------------------
/**
 * Absolute TelAF app installation path which is used as overlay work dir.
 */
//--------------------------------------------------------------------------------------------------
#define TELAF_APP_OVERLAYFS_WK TELAF_APP_PATH"/appsInstall_wk"

//--------------------------------------------------------------------------------------------------
/**
 * Absolute path to be used for mounting "/legato/systems/current" to overlay fs.
 */
//--------------------------------------------------------------------------------------------------
#define TELAF_APP_OVERLAYFS_UPPER_CURRENT_PATH TELAF_APP_OVERLAYFS_UPPER_PATH"/current"
#define TELAF_APP_OVERLAYFS_WK_CURRENT_PATH TELAF_APP_OVERLAYFS_WK"/current"

//--------------------------------------------------------------------------------------------------
/**
 * Absolute path to be used for mounting "/legato/apps" to overlay fs.
 */
//--------------------------------------------------------------------------------------------------
#define TELAF_APP_OVERLAYFS_UPPER_APPS_PATH TELAF_APP_OVERLAYFS_UPPER_PATH"/apps"
#define TELAF_APP_OVERLAYFS_WK_APPS_PATH TELAF_APP_OVERLAYFS_WK"/apps"

//--------------------------------------------------------------------------------------------------
/**
 * The path to the directory containing the current running "system".
 */
//--------------------------------------------------------------------------------------------------
#define CURRENT_SYSTEM_PATH        SYSTEM_PATH"/current"

//--------------------------------------------------------------------------------------------------
/**
 * The location where all applications are installed.
 */
//--------------------------------------------------------------------------------------------------
#define APPS_INSTALL_DIR            CURRENT_SYSTEM_PATH"/apps"

//--------------------------------------------------------------------------------------------------
/**
 * The writeable location for all installed applications.
 */
//--------------------------------------------------------------------------------------------------
#define APPS_WRITEABLE_DIR          CURRENT_SYSTEM_PATH"/appsWriteable"

//--------------------------------------------------------------------------------------------------
/**
 * The location where all system binaries are installed.
 */
//--------------------------------------------------------------------------------------------------
#define SYSTEM_BIN_PATH             CURRENT_SYSTEM_PATH"/bin"

//--------------------------------------------------------------------------------------------------
/**
 * The location where all kernel modules are installed.
 */
//--------------------------------------------------------------------------------------------------
#define SYSTEM_MODULE_PATH          CURRENT_SYSTEM_PATH"/modules"

//--------------------------------------------------------------------------------------------------
/**
 * The location of files, directories and scripts for kernel modules.
 */
//--------------------------------------------------------------------------------------------------
#define SYSTEM_MODULE_FILES_PATH    CURRENT_SYSTEM_PATH"/modules/files"

//--------------------------------------------------------------------------------------------------
/**
 * The location of config tree directory.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_TREE_PATH               CURRENT_SYSTEM_PATH"/config"


//--------------------------------------------------------------------------------------------------
/**
 * The location of read-only flag.
 */
//--------------------------------------------------------------------------------------------------
#define READ_ONLY_FLAG_PATH        "/mnt/legato/systems/current/read-only"


//--------------------------------------------------------------------------------------------------
/**
 * The location of boot count file. This file is only used to check whether device enter into a
 * reboot loop.
 */
//--------------------------------------------------------------------------------------------------
#define BOOT_COUNT_PATH            "/legato/bootCount"


//--------------------------------------------------------------------------------------------------
/**
 * Constant to use as a symlink target (in place of MD5-based directory name), when the app is
 * preloaded and its version may not match the latest version.
 */
//--------------------------------------------------------------------------------------------------
#define PRELOADED_ANY_VERSION "PRELOADED_ANY_VERSION"


//--------------------------------------------------------------------------------------------------
/**
 * Default prefix path for legato file system.
 */
//--------------------------------------------------------------------------------------------------
#define FS_PREFIX_DATA_PATH      "/data/le_fs"


#endif  // LEGATO_SYSPATHS_INCLUDE_GUARD

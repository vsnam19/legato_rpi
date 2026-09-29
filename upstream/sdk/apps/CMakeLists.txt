cmake_minimum_required(VERSION 3.10.2)

project (telematics-apps)

# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)
# pkg-config module
include(FindPkgConfig)

set(ENV{PKG_CONFIG_PATH} "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/:$ENV{PKG_CONFIG_PATH}")

pkg_check_modules(TELUX_TEL_LIB telux-tel)
pkg_check_modules(TELUX_CV2X_LIB telux-cv2x)
pkg_check_modules(TELUX_V2X_RADIO_LIB v2x-radio)
pkg_check_modules(TELUX_CONNECT_LIB telux-connect)
pkg_check_modules(TELUX_SENSOR_LIB telux-sensor)
pkg_check_modules(TELUX_SECURITY_LIB telux-sec)
pkg_check_modules(TELUX_POWER_LIB telux-power)
pkg_check_modules(TELUX_PLATFORM_LIB telux-platform)
pkg_check_modules(TELUX_PLATFORM_DIAG_LIB telux-platform-diag)
pkg_check_modules(TELUX_DATA_LIB telux-data)
pkg_check_modules(TELUX_WLAN_LIB telux-wlan)
pkg_check_modules(TELUX_AUDIO_LIB telux-audio)
pkg_check_modules(TELUX_THERM_LIB telux-therm)
pkg_check_modules(TELUX_LOC_LIB telux-loc)
pkg_check_modules(TELUX_CONFIG_LIB telux-config)
pkg_check_modules(TELUX_SEC_CON_LIB telux-sec-con)
pkg_check_modules(TELUX_SQUISH_LIB telux_squish)
pkg_check_modules(TELUX_SATCOM_LIB telux-satcom)

if (TELUX_TEL_LIB_FOUND)
   set (TELSDK_FEATURE_TEL ON)
   add_definitions(-DTELSDK_FEATURE_TEL_ENABLED)
endif (TELUX_TEL_LIB_FOUND)

if (TELUX_CV2X_LIB_FOUND)
   set (TELSDK_FEATURE_CV2X ON)
   add_definitions(-DTELSDK_FEATURE_CV2X_ENABLED)
endif (TELUX_CV2X_LIB_FOUND)

if (TELUX_CONNECT_LIB_FOUND)
   set (TELSDK_FEATURE_CONNECT ON)
   add_definitions(-DTELSDK_FEATURE_CONNECT_ENABLED)
endif (TELUX_CONNECT_LIB_FOUND)

if (TELUX_SENSOR_LIB_FOUND)
   set (TELSDK_FEATURE_SENSOR ON)
   add_definitions(-DTELSDK_FEATURE_SENSOR_ENABLED)
endif (TELUX_SENSOR_LIB_FOUND)

if (TELUX_SECURITY_LIB_FOUND)
   set (TELSDK_FEATURE_CRYPTO ON)
   add_definitions(-DTELSDK_FEATURE_CRYPTO_ENABLED)
   set (TELSDK_FEATURE_CRYPTO_ACCELERATOR ON)
   add_definitions(-DTELSDK_FEATURE_CRYPTO_ACCELERATOR_ENABLED)
endif (TELUX_SECURITY_LIB_FOUND)

if (TELUX_SEC_CON_LIB_FOUND)
   set (TELSDK_FEATURE_CONNECTION_SECURITY ON)
   add_definitions(-DTELSDK_FEATURE_CONNECTION_SECURITY_ENABLED)
endif (TELUX_SEC_CON_LIB_FOUND)

if (TELUX_POWER_LIB_FOUND)
   set (TELSDK_FEATURE_POWER ON)
   add_definitions(-DTELSDK_FEATURE_POWER_ENABLED)
endif (TELUX_POWER_LIB_FOUND)

if (TELUX_PLATFORM_LIB_FOUND)
   set (TELSDK_FEATURE_PLATFORM ON)
   add_definitions(-DTELSDK_FEATURE_PLATFORM_ENABLED)
endif (TELUX_PLATFORM_LIB_FOUND)

if (TELUX_PLATFORM_DIAG_LIB_FOUND)
   set (TELSDK_FEATURE_PLATFORM_DIAG ON)
   add_definitions(-DTELSDK_FEATURE_PLATFORM_DIAG_ENABLED)
endif (TELUX_PLATFORM_DIAG_LIB_FOUND)

if (TELUX_DATA_LIB_FOUND)
   set (TELSDK_FEATURE_DATA ON)
   add_definitions(-DTELSDK_FEATURE_DATA_ENABLED)
endif (TELUX_DATA_LIB_FOUND)

if (TELUX_WLAN_LIB_FOUND)
   set (TELSDK_FEATURE_WIFI ON)
   add_definitions(-DTELSDK_FEATURE_WLAN_ENABLED)
endif (TELUX_WLAN_LIB_FOUND)

if (TELUX_AUDIO_LIB_FOUND)
   set (TELSDK_FEATURE_AUDIO ON)
   add_definitions(-DTELSDK_FEATURE_AUDIO_ENABLED)
   add_definitions(-DAUDIO_SUPPORTED)
endif (TELUX_AUDIO_LIB_FOUND)

if (TELUX_THERM_LIB_FOUND)
   set (TELSDK_FEATURE_THERM ON)
   add_definitions(-DTELSDK_FEATURE_THERM_ENABLED)
endif (TELUX_THERM_LIB_FOUND)

if (TELUX_LOC_LIB_FOUND)
   set (TELSDK_FEATURE_LOC ON)
   add_definitions(-DTELSDK_FEATURE_LOC_ENABLED)
endif (TELUX_LOC_LIB_FOUND)

if (TELUX_CONFIG_LIB_FOUND)
   set (TELSDK_FEATURE_CONFIG ON)
   add_definitions(-DTELSDK_FEATURE_CONFIG_ENABLED)
endif (TELUX_CONFIG_LIB_FOUND)

if (MACHINE_HAS_CV2X_ONLY)
    add_definitions(-DFEATURE_CV2X_ONLY)
endif (MACHINE_HAS_CV2X_ONLY)

if(TELSDK_FEATURE_FOR_SECONDARY_VM)
    add_definitions(-DTELSDK_FEATURE_FOR_SECONDARY_VM_ENABLED)
endif (TELSDK_FEATURE_FOR_SECONDARY_VM)

if (TELUX_FOR_EXTERNAL_AP)
    add_definitions(-DTELUX_FOR_EXTERNAL_AP)
    if (TELUX_QTI_EXTERNAL_AP)
        add_definitions(-DTELUX_QTI_EXTERNAL_AP)
    endif (TELUX_QTI_EXTERNAL_AP)
endif (TELUX_FOR_EXTERNAL_AP)

if (TELUX_SQUISH_LIB_FOUND)
   set (TELSDK_FEATURE_SQUISH ON)
   add_definitions(-DTELSDK_FEATURE_SQUISH_ENABLED)
endif (TELUX_SQUISH_LIB_FOUND)

if (TELUX_SATCOM_LIB_FOUND)
    set (TELSDK_FEATURE_SATCOM ON)
    add_definitions(-DTELSDK_QMS_SUPPORT_ENABLED)
endif (TELUX_SATCOM_LIB_FOUND)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

#To enable debug symbols, set -DENABLE_DEBUG=ON
if(ENABLE_DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

# build subdirectories
add_subdirectory( common/console_app_framework )
add_subdirectory( reference )
add_subdirectory( tests )
add_subdirectory( samples)

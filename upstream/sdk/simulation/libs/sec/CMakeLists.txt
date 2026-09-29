# libtelux_sec.so:
# Security support in TelSDK

cmake_minimum_required(VERSION 3.10.2)

# log filter
add_definitions(-DTELUX_TECH_AREA=10)

# build artifact libtelux_sec.so
set(TARGET_LIBTELUX_SEC telux_sec)

configure_file("telux-sec.pc.in" "telux-sec.pc" @ONLY)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

set(LIBTELUX_SEC_SRCS
    SecurityFactoryImpl.cpp
    CryptoParamBuilder.cpp
    CryptoParamImpl.cpp
    CryptoManagerImpl.cpp
    CryptoAcceleratorManagerImpl.cpp
    CryptoAcceleratorUtils.cpp
    CryptoAcceleratorResultParser.cpp
    CAControlManagerImpl.cpp
    RandomNumberManagerImpl.cpp)

add_library(${TARGET_LIBTELUX_SEC} SHARED
    ${LIBTELUX_SEC_SRCS})

# libraries to link against
target_link_libraries(${TARGET_LIBTELUX_SEC}
    telux_common
    telux_protos)

# install to target
install(TARGETS ${TARGET_LIBTELUX_SEC}
         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-sec.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")

# Target libtelux_sec_con.so
set(TARGET_LIBTELUX_SECCON telux_sec_con)

configure_file("telux-sec-con.pc.in" "telux-sec-con.pc" @ONLY)

set(LIBTELUX_SECCON_SRCS
    ConnectionSecurityFactoryImpl.cpp
    CellularSecurityManagerImpl.cpp
    WiFiSecurityManagerImpl.cpp)

add_library(${TARGET_LIBTELUX_SECCON} SHARED
    ${LIBTELUX_SECCON_SRCS})

target_link_libraries(${TARGET_LIBTELUX_SECCON}
    telux_common
    telux_protos)

install(TARGETS ${TARGET_LIBTELUX_SECCON}
         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-sec-con.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")

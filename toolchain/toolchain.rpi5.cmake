# ==============================================================================
# CMake Toolchain Configuration for Raspberry Pi 5 (AArch64)
# ==============================================================================
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross compiler binaries
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CMAKE_AR aarch64-linux-gnu-ar CACHE FILEPATH "Archiver")
set(CMAKE_STRIP aarch64-linux-gnu-strip CACHE FILEPATH "Strip")
set(CMAKE_RANLIB aarch64-linux-gnu-ranlib CACHE FILEPATH "Ranlib")

# CPU tuning for Broadcom BCM2712 (Cortex-A76)
set(RPI5_C_FLAGS "-march=armv8-a+crc+crypto -mtune=cortex-a76")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${RPI5_C_FLAGS}" CACHE STRING "C flags" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${RPI5_C_FLAGS}" CACHE STRING "C++ flags" FORCE)

# Search paths policy
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

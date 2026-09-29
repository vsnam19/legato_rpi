cmake_minimum_required(VERSION 3.16)

project(TAF_PA_COMMON C CXX)

set(LIBRARY_VERSION "1.0.0")
set(LIBRARY_SOVERSION "1")
set(TARGET_NAME taf_pa_commonDef)
set(OUTPUT_NAME Component_${TARGET_NAME})

# common headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)

add_library(${TARGET_NAME} SHARED taf_pa_common.c)

set_target_properties(${TARGET_NAME} PROPERTIES
    VERSION ${LIBRARY_VERSION}
    SOVERSION ${LIBRARY_SOVERSION}
    OUTPUT_NAME ${OUTPUT_NAME}
)

install(TARGETS ${TARGET_NAME}
    LIBRARY DESTINATION lib
)

# Enable macro TAF_PA_DEFAULT for default PA
target_compile_definitions(${TARGET_NAME} PRIVATE TAF_PA_DEFAULT=1)

# -------------------------------------------
# Add stub library: libtaf_pa_common.so
# -------------------------------------------
set(STUB_TARGET_NAME taf_pa_common)
set(STUB_OUTPUT_NAME Component_${STUB_TARGET_NAME})

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty.cpp "// empty stub source\n")

add_library(${STUB_TARGET_NAME} SHARED ${CMAKE_CURRENT_BINARY_DIR}/empty.cpp)

set_target_properties(${STUB_TARGET_NAME} PROPERTIES
    VERSION ${LIBRARY_VERSION}
    SOVERSION ${LIBRARY_SOVERSION}
    STUB_OUTPUT_NAME ${STUB_TARGET_NAME}
)

install(TARGETS ${STUB_TARGET_NAME}
    LIBRARY DESTINATION lib
)

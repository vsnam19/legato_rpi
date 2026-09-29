cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(taf_pa_pmsDef SHARED taf_pa_pms.cpp)

target_include_directories(
    taf_pa_pmsDef
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(taf_pa_pmsDef PRIVATE taf_pa_commonDef)

set_target_properties(
    taf_pa_pmsDef
    PROPERTIES
        OUTPUT_NAME Component_taf_pa_pmsDef)

install(TARGETS taf_pa_pmsDef LIBRARY DESTINATION lib)

# Enable macro TAF_PA_DEFAULT for default PA
target_compile_definitions(taf_pa_pmsDef PRIVATE TAF_PA_DEFAULT=1)

# -------------------------------------------
# Add stub library: Component_taf_pa_pms.so
# -------------------------------------------

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty.cpp "// empty stub source\n")

add_library(taf_pa_pms SHARED ${CMAKE_CURRENT_BINARY_DIR}/empty.cpp)

set_target_properties(taf_pa_pms PROPERTIES OUTPUT_NAME Component_taf_pa_pms)

install(TARGETS taf_pa_pms LIBRARY DESTINATION lib)

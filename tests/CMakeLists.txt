cmake_minimum_required(VERSION 3.16)
project(CfgManagerTests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_COVERAGE "Enable code coverage testing" ON)
if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage --coverage -O0 -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

find_package(GTest REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/crypto/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/security/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/storage/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/events/include
    ${OPENSSL_INCLUDE_DIR}
)

file(GLOB_RECURSE CORE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/core/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/crypto/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/security/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/storage/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/events/src/*.cpp
)

file(GLOB_RECURSE TEST_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_executable(cfgmanager_tests
    ${TEST_SRCS}
    ${CORE_SRCS}
)

target_link_libraries(cfgmanager_tests
    PRIVATE
    GTest::GTest
    GTest::Main
    OpenSSL::Crypto
    Threads::Threads
)

enable_testing()
add_test(NAME CfgManagerAllTests COMMAND cfgmanager_tests)

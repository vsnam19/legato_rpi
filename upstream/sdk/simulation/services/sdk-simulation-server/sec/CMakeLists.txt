cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SIMULATION_SERVER_APP_SRC
    sec/SecurityRNGServerImpl.cpp
    sec/SecurityCCSServerImpl.cpp
    sec/SecurityWCSServerImpl.cpp
    sec/SecurityCALCServerImpl.cpp
    sec/SecurityCryptoServerImpl.cpp
    sec/SecurityCryptoAcceleratorServerImpl.cpp
)

target_sources (${TARGET_SIMULATION_SERVER_APP} PRIVATE ${TARGET_SIMULATION_SERVER_APP_SRC})
cmake_minimum_required(VERSION 3.11)

project("fw-nrf-dfu")

#Define toolchain to by pass dependencies bootstrapping
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/toolchain_arm.cmake"
    CACHE PATH "CMAKETOOLCHAINFILE")

#Compilation parameters
set(COMPILE_CONNECTIVITY true)
set(DISABLE_TESTS true)
set(ARCH "arm")
set(SD_API_VER_NUMS 5)
set(SDK_V15_PATH ${CMAKE_CURRENT_SOURCE_DIR}/nRF5_SDK_15.3.0)
include (version.cmake)

#Call pc-ble-driver root directory
add_subdirectory(pc-ble-driver)

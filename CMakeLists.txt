cmake_minimum_required(VERSION 3.11)

#Compile DFU firmware only using a patched version of nRF5 SDK 15.3.0
set(COMPILE_CONNECTIVITY true)
set(DISABLE_TESTS true)
set(ARCH "arm")
set(SD_API_VER_NUMS 5)
set(SDK_V15_PATH ${CMAKE_CURRENT_SOURCE_DIR}/nRF5_SDK_15.3.0)

#DFU version. JDRF builds >= 100.0.0
set(CONNECTIVITY_VERSION "100.0.0")

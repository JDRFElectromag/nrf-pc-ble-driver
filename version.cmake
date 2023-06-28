cmake_minimum_required (VERSION 3.3.2)

#Connectivity is built by JDRF if FW >= 100.0.0.
set(MAJOR_VER 100)
set(MINOR_VER 0)
if(NOT DEFINED BUILD_NUM)
    set(BUILD_NUM 0)
endif()

set(CONNECTIVITY_VERSION "${MAJOR_VER}.${MINOR_VER}.${BUILD_NUM}")

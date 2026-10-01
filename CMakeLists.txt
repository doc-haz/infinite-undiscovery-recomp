# infinite_undiscovery - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(infinite_undiscovery LANGUAGES CXX)

if(WIN32)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(INFINITEUNDISCOVERY_SOURCES
    src/main.cpp
)

if(WIN32)
    list(APPEND INFINITEUNDISCOVERY_SOURCES src/infinite_undiscovery.rc)
endif()

if(WIN32)
    add_executable(infinite_undiscovery WIN32 ${INFINITEUNDISCOVERY_SOURCES})
else()
    add_executable(infinite_undiscovery ${INFINITEUNDISCOVERY_SOURCES})
endif()

rexglue_setup_target(infinite_undiscovery)

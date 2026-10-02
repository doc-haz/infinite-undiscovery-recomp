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
    src/asset_setup.cpp
    src/asset_dlc.cpp
    src/portable_setup.cpp
)

if(WIN32)
    list(APPEND INFINITEUNDISCOVERY_SOURCES src/infinite_undiscovery.rc)
endif()

option(IU_BUILD_SETUP_TESTS "Build local asset installer validation executable" OFF)
if(IU_BUILD_SETUP_TESTS)
    add_executable(iu_asset_setup_tests tests/asset_setup_tests.cpp src/asset_setup.cpp src/asset_dlc.cpp src/portable_setup.cpp)
    target_link_libraries(iu_asset_setup_tests PRIVATE rex::runtime bcrypt comctl32 ole32 shell32 uuid gdi32)
    target_include_directories(iu_asset_setup_tests PRIVATE src)
    rexglue_apply_target_settings(iu_asset_setup_tests)
endif()

if(WIN32)
    add_executable(infinite_undiscovery WIN32 ${INFINITEUNDISCOVERY_SOURCES})
else()
    add_executable(infinite_undiscovery ${INFINITEUNDISCOVERY_SOURCES})
endif()

rexglue_setup_target(infinite_undiscovery)
# Preserve the historical PoC executable; publish this stage under its new name.
set_target_properties(infinite_undiscovery PROPERTIES OUTPUT_NAME "InfiniteUndiscoveryRecomp")

if(WIN32)
    target_link_libraries(infinite_undiscovery PRIVATE bcrypt comctl32 ole32 shell32 uuid gdi32)
else()
    message(FATAL_ERROR "Asset Setup currently requires Windows; do not imply cross-platform support.")
endif()

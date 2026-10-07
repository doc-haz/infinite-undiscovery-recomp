# ReXGlue release guard.
#
# Verifies that the ReXGlue runtime DLLs that ship with Infinite Undiscovery
# Recomp are the auditable v0.10.0 build and not the old precompiled SDK copies.
#
# Usage:
#   cmake -DVERIFY_DIR=<dir> [-DREQUIRE_ALL=ON] -P cmake/verify_rexglue_dlls.cmake
#
# VERIFY_DIR : directory expected to contain rexruntime.dll / rexgpu-xenos.dll.
# REQUIRE_ALL: when ON, both DLLs must be present.
#
# The expected hashes come from the auditable checkout
#   rexglue-sdk-v0.10.0-git  (tag v0.10.0, commit
#   f5337cdc947ff6d4c4196737e2c807a48f2a1fc2)
# built for win-amd64 Release. The forbidden hashes are the copies produced by
# the old precompiled SDK that used to leak into the release.

if(NOT DEFINED VERIFY_DIR)
    message(FATAL_ERROR "verify_rexglue_dlls: pass -DVERIFY_DIR=<dir>")
endif()

set(_expect_rexruntime "25C0F2D1DBB7FE3147FC59E22C9A3E4C764DC2E0B0C6877FC86F0DA499E0F67F")
set(_expect_rexgpu     "98CEF22E2AC1667F3A42910DD7474C0409BBEF7DB7599B3A48DF1EDBB0739A2D")

set(_old_rexruntime "E359209FB2B0570E693C966D4C1D99A82465D36EF70D033833FAE56ADB2F1B7A")
set(_old_rexgpu     "0C23CFA23FC4FA5638DC8A3DC0DE87B1041D97705D4677774083F1F350CD8D89")

set(_found 0)

foreach(_name rexruntime.dll rexgpu-xenos.dll)
    if(_name STREQUAL "rexruntime.dll")
        set(_expect "${_expect_rexruntime}")
        set(_old "${_old_rexruntime}")
    else()
        set(_expect "${_expect_rexgpu}")
        set(_old "${_old_rexgpu}")
    endif()

    set(_file "${VERIFY_DIR}/${_name}")
    if(EXISTS "${_file}")
        file(SHA256 "${_file}" _hash)
        string(TOUPPER "${_hash}" _hash)
        if(_hash STREQUAL "${_old}")
            message(FATAL_ERROR
                "ReXGlue release guard: '${_name}' is the OLD precompiled-SDK DLL "
                "(sha256 ${_hash}). Aborting.\n"
                "Use the auditable ReXGlue v0.10.0 build "
                "(rexglue-sdk-v0.10.0-git, commit "
                "f5337cdc947ff6d4c4196737e2c807a48f2a1fc2).")
        endif()
        if(NOT _hash STREQUAL "${_expect}")
            message(FATAL_ERROR
                "ReXGlue release guard: '${_name}' has unexpected sha256 ${_hash}.\n"
                "Expected the auditable v0.10.0 build: ${_expect}")
        endif()
        message(STATUS "ReXGlue release guard: ${_name} OK (${_hash})")
        math(EXPR _found "${_found}+1")
    elseif(REQUIRE_ALL)
        message(FATAL_ERROR
            "ReXGlue release guard: required file '${_file}' is missing.")
    endif()
endforeach()

if(_found EQUAL 0)
    message(FATAL_ERROR
        "ReXGlue release guard: no ReXGlue DLLs found under ${VERIFY_DIR}")
endif()

# Copyright 2026 RethinkDB, all rights reserved.
#
# Version strings come from scripts/gen-version.sh. RETHINKDB_VERSION is the
# packaging override. When that override is set, the code version still comes
# from the script unless RETHINKDB_CODE_VERSION is set.

set(RETHINKDB_VERSION "" CACHE STRING
    "Packaging version override. Empty uses scripts/gen-version.sh.")
set(RETHINKDB_CODE_VERSION "" CACHE STRING
    "Code version override. Empty uses the script, or the packaging version when that was not overridden.")

set(_rethinkdb_version_deps
    "${CMAKE_SOURCE_DIR}/VERSION"
    "${CMAKE_SOURCE_DIR}/VERSION.OVERRIDE"
    "${CMAKE_SOURCE_DIR}/.git/HEAD"
)
foreach(_rethinkdb_version_dep IN LISTS _rethinkdb_version_deps)
    if(EXISTS "${_rethinkdb_version_dep}")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_rethinkdb_version_dep}")
    endif()
endforeach()

execute_process(
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/gen-version.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE _rethinkdb_git_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _rethinkdb_version_result
)
if(NOT _rethinkdb_version_result EQUAL 0 OR _rethinkdb_git_version STREQUAL "")
    message(FATAL_ERROR "scripts/gen-version.sh failed")
endif()

if(RETHINKDB_VERSION STREQUAL "")
    set(RETHINKDB_VERSION "${_rethinkdb_git_version}")
endif()

if(RETHINKDB_CODE_VERSION STREQUAL "")
    if(RETHINKDB_VERSION STREQUAL _rethinkdb_git_version)
        set(RETHINKDB_CODE_VERSION "${RETHINKDB_VERSION}")
    else()
        set(RETHINKDB_CODE_VERSION "${_rethinkdb_git_version}")
    endif()
endif()

string(REGEX MATCH "^[0-9]+\\.[0-9]+" RETHINKDB_SHORT_VERSION "${RETHINKDB_VERSION}")
if(RETHINKDB_SHORT_VERSION STREQUAL "")
    set(RETHINKDB_SHORT_VERSION "0.0")
endif()

string(TIMESTAMP RETHINKDB_BUILD_DATE "%Y-%m-%d")

if(MSVC)
    set(RETHINKDB_BUILD_MACHINE "Windows")
else()
    execute_process(
        COMMAND "${CMAKE_CXX_COMPILER}" -dumpmachine
        OUTPUT_VARIABLE RETHINKDB_BUILD_MACHINE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _rethinkdb_machine_result
    )
    if(NOT _rethinkdb_machine_result EQUAL 0 OR RETHINKDB_BUILD_MACHINE STREQUAL "")
        set(RETHINKDB_BUILD_MACHINE "${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

message(STATUS "RethinkDB version ${RETHINKDB_VERSION} (code ${RETHINKDB_CODE_VERSION})")

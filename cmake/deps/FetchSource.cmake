# Copyright 2026 RethinkDB, all rights reserved.
#
# Download and extract a source archive at configure time. Does not
# add_subdirectory, so header-only and hand-built trees stay out of the
# project. Sets <name>_SOURCE_DIR in the caller.

function(rethinkdb_fetch_source name)
    cmake_parse_arguments(ARG "" "URL;URL_HASH" "" ${ARGN})
    if(NOT ARG_URL OR NOT ARG_URL_HASH)
        message(FATAL_ERROR "rethinkdb_fetch_source(${name}) requires URL and URL_HASH")
    endif()

    FetchContent_Declare("${name}"
        URL "${ARG_URL}"
        URL_HASH "${ARG_URL_HASH}"
        DOWNLOAD_DIR "${RETHINKDB_DOWNLOAD_DIR}"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_GetProperties("${name}")

    if(NOT ${name}_POPULATED)
        # Header-only and hand-built trees must not be add_subdirectory'd.
        if(POLICY CMP0169)
            cmake_policy(SET CMP0169 OLD)
        endif()
        FetchContent_Populate("${name}")
    endif()

    set("${name}_SOURCE_DIR" "${${name}_SOURCE_DIR}" PARENT_SCOPE)
endfunction()

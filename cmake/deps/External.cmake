# Copyright 2026 RethinkDB, all rights reserved.
#
# Shared download location and ExternalProject log settings.

if(FETCHCONTENT_BASE_DIR)
    set(RETHINKDB_DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}")
else()
    set(RETHINKDB_DOWNLOAD_DIR "${CMAKE_SOURCE_DIR}/.cache/fetchcontent")
endif()

file(MAKE_DIRECTORY "${RETHINKDB_DOWNLOAD_DIR}")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/external/logs")

set(RETHINKDB_EXTERNAL_LOG_ARGS
    LOG_DIR "${CMAKE_BINARY_DIR}/external/logs"
    LOG_DOWNLOAD TRUE
    LOG_PATCH TRUE
    LOG_CONFIGURE TRUE
    LOG_BUILD TRUE
    LOG_INSTALL TRUE
    LOG_OUTPUT_ON_FAILURE TRUE
    UPDATE_COMMAND ""
)

find_program(RETHINKDB_GNU_MAKE NAMES gmake make
    HINTS /opt/homebrew/bin /usr/local/bin
    REQUIRED)

function(rethinkdb_record_dist_source name source_dir)
    file(APPEND "${CMAKE_BINARY_DIR}/dist-support.txt" "${name}\t${source_dir}\n")
endfunction()

function(rethinkdb_external_dependency target)
    set_property(GLOBAL APPEND PROPERTY RETHINKDB_EXTERNAL_DEPS "${target}")
endfunction()

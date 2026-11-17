# Copyright 2026 RethinkDB, all rights reserved.
#
# Build zlib for Windows.

if(NOT SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR is required")
endif()

if(NOT CONFIG)
    set(CONFIG Release)
endif()

find_program(NMAKE_EXECUTABLE nmake REQUIRED)

set(runtime "-MT")
if(CONFIG MATCHES "^[Dd]ebug")
    set(runtime "-MTd")
endif()

execute_process(
    COMMAND "${NMAKE_EXECUTABLE}" -f win32/Makefile.msc clean all "RUNTIME=${runtime}"
    WORKING_DIRECTORY "${SOURCE_DIR}"
    RESULT_VARIABLE build_result
)

if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "zlib nmake failed")
endif()

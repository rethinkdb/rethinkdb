# Copyright 2026 RethinkDB, all rights reserved.
#
# Build curl for Windows.

if(NOT SOURCE_DIR OR NOT MACHINE)
    message(FATAL_ERROR "SOURCE_DIR and MACHINE are required")
endif()

if(NOT CONFIG)
    set(CONFIG Release)
endif()

find_program(NMAKE_EXECUTABLE nmake REQUIRED)

set(flags "")
if(CONFIG MATCHES "^[Dd]ebug")
    set(flags "DEBUG=yes")
endif()

execute_process(
    COMMAND "${NMAKE_EXECUTABLE}" /f Makefile.vc mode=static
        "MACHINE=${MACHINE}" RTLIBCFG=static ${flags}
    WORKING_DIRECTORY "${SOURCE_DIR}/winbuild"
    RESULT_VARIABLE build_result
)

if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "curl nmake failed")
endif()

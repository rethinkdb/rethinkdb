# Copyright 2026 RethinkDB, all rights reserved.
#
# Build the pinned OpenSSL with Strawberry Perl and nmake. CONFIG is Debug or
# Release. TARGET is VC-WIN32 or VC-WIN64A.

if(NOT DEFINED SOURCE_DIR OR NOT DEFINED TARGET)
    message(FATAL_ERROR "SOURCE_DIR and TARGET are required")
endif()

if(NOT CONFIG)
    set(CONFIG Release)
endif()

find_program(PERL_EXECUTABLE perl REQUIRED)
find_program(NMAKE_EXECUTABLE nmake REQUIRED)

set(openssl_target "${TARGET}")
if(CONFIG MATCHES "^[Dd]ebug")
    set(openssl_target "debug-${TARGET}")
endif()

execute_process(
    COMMAND "${PERL_EXECUTABLE}" Configure "${openssl_target}" no-asm no-shared
    WORKING_DIRECTORY "${SOURCE_DIR}"
    RESULT_VARIABLE configure_result
)

if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "OpenSSL Configure failed")
endif()

execute_process(
    COMMAND "${NMAKE_EXECUTABLE}"
    WORKING_DIRECTORY "${SOURCE_DIR}"
    RESULT_VARIABLE build_result
)

if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "OpenSSL nmake failed")
endif()

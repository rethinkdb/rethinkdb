# Copyright 2026 RethinkDB, all rights reserved.
#
# Check if the binary links in jemalloc.

if(NOT BINARY)
    message(FATAL_ERROR "BINARY is required")
endif()

execute_process(
    COMMAND objdump -T "${BINARY}"
    RESULT_VARIABLE dump_result
    OUTPUT_VARIABLE dump_output
)

if(NOT dump_result EQUAL 0)
    message(FATAL_ERROR "objdump failed while checking jemalloc")
endif()

if(NOT dump_output MATCHES "mallctlnametomib")
    message(FATAL_ERROR "Failed to link in jemalloc.")
endif()

# Copyright 2026 RethinkDB, all rights reserved.
#
# Helper to gzip a file.

find_program(GZIP_EXECUTABLE gzip REQUIRED)

if(NOT IN OR NOT OUT)
    message(FATAL_ERROR "gzip_file.cmake requires IN and OUT")
endif()

execute_process(
    COMMAND "${GZIP_EXECUTABLE}" -9 -c "${IN}"
    OUTPUT_FILE "${OUT}"
    COMMAND_ERROR_IS_FATAL ANY
)

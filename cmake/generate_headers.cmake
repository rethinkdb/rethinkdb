# Copyright 2026 RethinkDB, all rights reserved.
#
# Generate the headers for the server.

if(NOT PYTHON OR NOT ROOT)
    message(FATAL_ERROR "PYTHON and ROOT are required")
endif()

execute_process(
    COMMAND "${PYTHON}" "${ROOT}/scripts/generate_serialize_macros.py"
    OUTPUT_FILE "${ROOT}/src/rpc/serialize_macros.hpp"
    COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
    COMMAND "${PYTHON}" "${ROOT}/scripts/generate_join_macros.py"
    OUTPUT_FILE "${ROOT}/src/rpc/semilattice/joins/macros.hpp"
    COMMAND_ERROR_IS_FATAL ANY
)

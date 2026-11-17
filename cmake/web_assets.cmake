# Copyright 2026 RethinkDB, all rights reserved.
#
# Compile the web assets used by the admin UI.

if(NOT PYTHON OR NOT OUT)
    message(FATAL_ERROR "PYTHON and OUT are required")
endif()

if(NOT ASSETS OR NOT IS_DIRECTORY "${ASSETS}")
    message(FATAL_ERROR
        "Set RETHINKDB_WEB_ASSETS_DIR to the admin UI directory before building the web-assets target")
endif()

execute_process(
    COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/../scripts/compile-web-assets.py" "${ASSETS}"
    OUTPUT_FILE "${OUT}"
    COMMAND_ERROR_IS_FATAL ANY
)

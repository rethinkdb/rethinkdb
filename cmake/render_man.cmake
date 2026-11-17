# Copyright 2026 RethinkDB, all rights reserved.
#
# Render a man page.

find_program(M4_EXECUTABLE m4 REQUIRED)

if(NOT OUT OR NOT M4 OR NOT SHORT_VERSION OR NOT DATE)
    message(FATAL_ERROR "render_man.cmake requires M4, OUT, SHORT_VERSION, and DATE")
endif()

get_filename_component(_man_dir "${OUT}" DIRECTORY)
file(MAKE_DIRECTORY "${_man_dir}")
execute_process(
    COMMAND "${M4_EXECUTABLE}"
        "-D" "SHORT_VERSION=${SHORT_VERSION}"
        "-D" "CURRENT_DATE=${DATE}"
        "${M4}"
    OUTPUT_FILE "${OUT}"
    COMMAND_ERROR_IS_FATAL ANY
)

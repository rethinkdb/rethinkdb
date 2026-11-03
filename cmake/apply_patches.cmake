# Copyright 2026 RethinkDB, all rights reserved.
#
# Apply a CMake list of unified diffs. Re-running a patch that is already
# applied is not an error. A patch that does not apply either way fails.

find_program(PATCH_EXECUTABLE patch REQUIRED)

if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR is required")
endif()

foreach(patch IN LISTS PATCHES)
    execute_process(
        COMMAND "${PATCH_EXECUTABLE}" -p1 --forward --input "${patch}"
        WORKING_DIRECTORY "${SOURCE_DIR}"
        RESULT_VARIABLE forward_result
        OUTPUT_VARIABLE forward_output
        ERROR_VARIABLE forward_error
    )

    if(forward_result EQUAL 0)
        continue()
    endif()

    execute_process(
        COMMAND "${PATCH_EXECUTABLE}" -p1 --reverse --dry-run --input "${patch}"
        WORKING_DIRECTORY "${SOURCE_DIR}"
        RESULT_VARIABLE reverse_result
        OUTPUT_QUIET
        ERROR_QUIET
    )

    if(NOT reverse_result EQUAL 0)
        message(FATAL_ERROR
            "Failed to apply ${patch} in ${SOURCE_DIR}\n${forward_output}${forward_error}")
    endif()
endforeach()

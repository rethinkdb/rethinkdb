#!/bin/bash
# Copyright 2026 RethinkDB, all rights reserved.
#
# Print the CMAKE_SYSTEM_NAME recorded for an existing CMake build.
# CMake 3.28 and newer keep that name in CMakeFiles/<version>/CMakeSystem.cmake.
# A cache entry is accepted when the platform file is absent. Windows writes
# those files with CRLF, so a trailing carriage return is ignored.

cmake_system_name() {
    local binary_dir=$1
    local cache="${binary_dir}/CMakeCache.txt"
    local major=""
    local minor=""
    local patch=""
    local system_file=""
    local name=""

    if [[ ! -f "${cache}" ]]; then
        return 1
    fi

    major=$(sed -n 's/^CMAKE_CACHE_MAJOR_VERSION:[^=]*=//p' "${cache}")
    minor=$(sed -n 's/^CMAKE_CACHE_MINOR_VERSION:[^=]*=//p' "${cache}")
    patch=$(sed -n 's/^CMAKE_CACHE_PATCH_VERSION:[^=]*=//p' "${cache}")
    major=${major%%[[:cntrl:]]*}
    minor=${minor%%[[:cntrl:]]*}
    patch=${patch%%[[:cntrl:]]*}

    if [[ -n "${major}" && -n "${minor}" && -n "${patch}" ]]; then
        system_file="${binary_dir}/CMakeFiles/${major}.${minor}.${patch}/CMakeSystem.cmake"
        if [[ -f "${system_file}" ]]; then
            name=$(sed -n 's/^set(CMAKE_SYSTEM_NAME "\([^"]*\)").*/\1/p' "${system_file}")
            name=${name%%[[:cntrl:]]*}
            if [[ -z "${name}" ]]; then
                return 1
            fi
            printf '%s\n' "${name}"
            return 0
        fi
    fi

    name=$(sed -n 's/^CMAKE_SYSTEM_NAME:[A-Za-z][A-Za-z]*=//p' "${cache}")
    name=${name%%[[:cntrl:]]*}
    if [[ -z "${name}" ]]; then
        return 1
    fi
    printf '%s\n' "${name}"
}

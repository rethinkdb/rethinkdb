#!/bin/bash
# Copyright 2026 RethinkDB, all rights reserved.
#
# Package an existing Linux build as an RPM. This script does not configure
# or compile. The CMake package-rpm target builds the server first.

set -euo pipefail

# shellcheck source=cmake-system-name.sh
source "$(
    unset CDPATH
    cd -- "$(dirname -- "$0")" && pwd
)/cmake-system-name.sh"

binary_dir="${1:-${BINARY_DIR:-}}"
if [[ -z "${binary_dir}" ]]; then
    printf 'BINARY_DIR is required\n' >&2
    exit 1
fi

cache="${binary_dir}/CMakeCache.txt"
server="${binary_dir}/rethinkdb"
cpack_cmd="${CPACK:-cpack}"
output="${binary_dir}/packages/rpm"
system_name=""

if [[ ! -f "${cache}" ]]; then
    printf 'Not a CMake build directory: %s\n' "${binary_dir}" >&2
    exit 1
fi
system_name=$(cmake_system_name "${binary_dir}" || true)
if [[ "${system_name}" != "Linux" ]]; then
    printf 'RPM packages require a Linux CMake build: %s\n' "${binary_dir}" >&2
    exit 1
fi
if [[ ! -f "${binary_dir}/CPackConfig.cmake" ]]; then
    printf 'CPack configuration is missing: %s/CPackConfig.cmake\n' \
        "${binary_dir}" >&2
    exit 1
fi
if [[ -n "${CONFIG:-}" && -x "${binary_dir}/${CONFIG}/rethinkdb" ]]; then
    server="${binary_dir}/${CONFIG}/rethinkdb"
fi
if [[ ! -x "${server}" ]]; then
    printf 'Built server is missing: %s\n' "${server}" >&2
    exit 1
fi

rm -rf "${output}"
mkdir -p "${output}"
cpack_args=(
    -G RPM
    -B "${output}"
    --config "${binary_dir}/CPackConfig.cmake"
)
if [[ -n "${CONFIG:-}" ]]; then
    cpack_args+=(-C "${CONFIG}")
fi
"${cpack_cmd}" "${cpack_args[@]}"

#!/bin/bash
# Copyright 2026 RethinkDB, all rights reserved.
#
# Stage an existing CMake build and build the macOS disk image.
# This script does not configure or compile.

set -euo pipefail

# shellcheck source=cmake-system-name.sh
source "$(
    unset CDPATH
    cd -- "$(dirname -- "$0")" && pwd
)/cmake-system-name.sh"

binary_dir="${1:?build directory}"
prefix="${2:?install prefix}"
bindir="${3:?bin directory}"
config="${4:-${CONFIG:-}}"
source_dir=$(
    unset CDPATH
    cd -- "$(dirname -- "$0")/.." && pwd
)

if [[ -n "${CMAKE:-}" ]]; then
    cmake_cmd=${CMAKE}
else
    cmake_cmd=cmake
fi

cache="${binary_dir}/CMakeCache.txt"
system_name=""
if [[ ! -f "${cache}" ]]; then
    printf 'Not a CMake build directory: %s\n' "${binary_dir}" >&2
    exit 1
fi
system_name=$(cmake_system_name "${binary_dir}" || true)
if [[ "${system_name}" != "Darwin" ]]; then
    printf 'macOS packages require a Darwin CMake build: %s\n' "${binary_dir}" >&2
    exit 1
fi

stage="${binary_dir}/packages/osx/pkg"
server_bin="${stage}${prefix}/${bindir}"
output="${binary_dir}/packages/osx/rethinkdb.dmg"

rm -rf "${binary_dir}/packages/osx"
mkdir -p "${stage}"

install_args=(--install "${binary_dir}")
if [[ -n "${config}" ]]; then
    install_args+=(--config "${config}")
fi
DESTDIR="${stage}" "${cmake_cmd}" "${install_args[@]}"

if [[ ! -x "${server_bin}/rethinkdb" ]]; then
    printf 'Installed server is missing: %s/rethinkdb\n' "${server_bin}" >&2
    exit 1
fi

if ! command -v otool >/dev/null 2>&1; then
    printf 'otool is required to check the macOS package\n' >&2
    exit 1
fi

otool -L "${server_bin}/rethinkdb" \
    | awk '/^\t/ { sub(/ \(.+\)/, ""); print }' \
    | while read -r linked; do
        case "${linked}" in
            "${binary_dir}"/*)
                printf 'rethinkdb links to a build-tree library: %s\n' \
                    "${linked}" >&2
                exit 1
                ;;
        esac
    done

python3 "${source_dir}/packaging/osx/create_dmg.py" \
    --server-root "${server_bin}" \
    --install-path "${prefix}/${bindir}" \
    --ouptut-location "${output}"

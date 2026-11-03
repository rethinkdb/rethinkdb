#!/bin/bash
# Copyright 2026 RethinkDB, all rights reserved.
#
# Install into a staging prefix and build the macOS disk image.
set -euo pipefail

binary_dir="${1:?build directory}"
prefix="${2:?install prefix}"
bindir="${3:?bin directory}"
source_dir="$(cd "$(dirname "$0")/.." && pwd)"
stage="${binary_dir}/packages/osx/pkg"
server_bin="${stage}${prefix}/${bindir}"

rm -rf "${binary_dir}/packages/osx"
mkdir -p "${stage}"
DESTDIR="${stage}" cmake --install "${binary_dir}"

if [[ ! -x "${server_bin}/rethinkdb" ]]; then
    printf 'Installed server is missing: %s/rethinkdb\n' "${server_bin}" >&2
    exit 1
fi

/usr/bin/otool -L "${server_bin}/rethinkdb" | awk '/^\t/ { sub(/ \(.+\)/, ""); print }' | while read -r linked; do
    case "${linked}" in
        "${binary_dir}"/*)
            printf 'rethinkdb links to a non-system library: %s\n' "${linked}" >&2
            exit 1
            ;;
    esac
done

python3 "${source_dir}/packaging/osx/create_dmg.py" \
    --server-root "${server_bin}" \
    --install-path "${prefix}/${bindir}" \
    --ouptut-location "${binary_dir}/packages/osx/rethinkdb.dmg"

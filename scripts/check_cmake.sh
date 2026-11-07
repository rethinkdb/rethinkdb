#!/bin/bash
# Copyright 2026 RethinkDB, all rights reserved.

# Check CMake files with cmakelint.
#
# - With no arguments, checks CMakeLists.txt and every *.cmake file under cmake/
# - Otherwise, checks only the files named on the command line
#
# Install the checker with:
#   python -m pip install 'cmakelint==1.4.3'
#
# .cmakelintrc in the repository root supplies the project's settings.

set -euo pipefail

if ! command -v cmakelint >/dev/null 2>&1; then
    printf '%s\n' "cmakelint is not installed. Install it with: python -m pip install 'cmakelint==1.4.3'" >&2
    exit 1
fi

root=$(
    unset CDPATH
    cd -- "$(dirname -- "$0")/.." && pwd
)
config="${root}/.cmakelintrc"

if [ "$#" -eq 0 ]; then
    (
        cd "$root"
        find CMakeLists.txt cmake -type f \( -name CMakeLists.txt -o -name '*.cmake' \) -print0 \
            | sort -z \
            | xargs -0 cmakelint --config="$config" --
    )
else
    cmakelint --config="$config" -- "$@"
fi

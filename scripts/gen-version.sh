#!/usr/bin/env bash
# Copyright 2010-2013 RethinkDB, all rights reserved.

set -euo pipefail

print_usage () {
    echo "Usage: $0 [-r|-h]"
    echo "  -r  Print number of revisions since v0.1 tag and exit"
    echo "  -h  Print help (this message)"
    echo "Otherwise generate a rethinkdb version number for this source tree"
}

main () {
    parse_opts "$@"

    root=$(dirname $0)/..
    export GIT_DIR="$root/.git"
    export GIT_WORK_TREE="$root"

    try_files "${VERSION_FILE:-}" "$root/VERSION.OVERRIDE" "./VERSION"

    if version=$(git_gen_version); then
        try_version "$version"
    fi

    if ! version=$(release_notes_version); then
        version=0.0-internal-unknown-fallback
    fi

    fallback_version=$version-fallback
    echo "$0: Warning: could not determine the version, using the default version '${fallback_version}'" >&2
    echo "$fallback_version"
}

lf='
'

try_files () {
    for file in "$@"; do
        if [[ -n "$file" && -f "$file" ]]; then
            try_version "$(cat $file)"
        fi
    done
}

try_version () {
    case "$1" in
        # If it is empty or contains newline or space, ignore it
        ""|*$lf*|*" "*) return;;
        # Otherwise strip a leading "v" and return it
        *) echo $1 | sed -e 's/^v//'; exit 0;;
    esac
}

parse_opts () {
    while getopts rsh opt; do
        case "$opt" in
            r) git rev-list v0.1..HEAD | wc -l; exit 0;;
            h) print_usage; exit 0;;
            ?) print_usage >&2; exit 1;;
        esac
    done
}

git_gen_version () {
    if version=$(git describe --tags --match "v[0-9]*" --abbrev=6 HEAD 2> /dev/null); then
        if git_is_dirty; then
            echo "$version-dirty"
        else
            echo "$version"
        fi
    else
        return 1
    fi
}

git_is_dirty () {
    git update-index -q --refresh > /dev/null || true
    [[ -n "$(git diff-index --name-only HEAD -- || true)" ]]
}

release_notes_version () {
    grep Release "$root/NOTES.md" | egrep -o '[0-9]+(\.[0-9]+)+' | head -n 1
}

main "$@"

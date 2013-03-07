#!/bin/bash
# Copyright 2010-2012 RethinkDB, all rights reserved.
#
# Command line arguments:
#   -r  Print number of revisions since v0.1 and exit
#   -s  Print the version in a short form
#
# Determining the version:
# 1. If the variable VERSION_FILE is set, it's the first word of that file
# 2. If we are in a git repository:
# 2.a. if VERSION.OVERRIDE file exists at the top of the repository, it's first word is the version (unless it's empty)
# 2.b. otherwise, use git-describe to generate a version number, using the closest 'v[0-9]*' tag
# 2.c. "-dirty" is appended to the version if there are local changes
# 3. otherwise, if VERSION file exists in current directory, it's first word is the version (unless it's empty)
# 4. otherwise, use default_version variable (defined below) as the version
# 5. 'v' at the beginning of the version number is stripped

set -euo pipefail

default_version=${FALLBACK_VERSION:-'0.0-internal-unknown'}-fallback

lf='
'

print_usage () {
  cat 1>&2 <<END_USAGE
Usage: $0 [-r|-s]
  -r  Print number of revisions since v0.1 tag and exit
  -s  Print the version in a short form
  -h  Print help (this message)
END_USAGE
}

short=0
while getopts rsh opt; do
    case "$opt" in
    r)      git rev-list v0.1..HEAD | wc -l; exit 0;;
    s)      short=1;;
    h)      print_usage; exit 1;;
    ?)      print_usage; exit 1;;
    esac
done

##### Find the top of the repository

start_dir="$(pwd)"

if [ -n "$(/bin/bash -c 'echo $VERSION_FILE')" ]; then # If VERSION_FILE is set, use it
    version_file="$VERSION_FILE"
elif [ -f "$(dirname "$0")/../VERSION.OVERRIDE" ]; then
    version_file="$(dirname "$0")/../VERSION.OVERRIDE"
else
    version_file="${start_dir}/VERSION"
fi

if repo_dir="$(git rev-parse --show-cdup 2> /dev/null)" && \
   repo_dir="${repo_dir:-.}" && \
   [ -d "$repo_dir/.git" ]; then

    repo_available=1
else
    repo_available=0
fi

#### Compute version

version=''

[ -f "${version_file}" ] && version=$(cat "$version_file")
if [ -z "$version" -a "$repo_available" = 1 ]; then
  version="$(git describe --tags --match "v[0-9]*" --abbrev=6 HEAD 2> /dev/null || true)"
  case "$version" in
    *$lf*)
      exit 1 ;;
    v[0-9]*)
      git update-index -q --refresh > /dev/null || true
      [ -n "$(git diff-index --name-only HEAD -- || true)" ] && version="${version}-dirty"
      ;;
  esac
fi


if [ -z "$version" ]; then
  echo "$0: Warning: could not determine the version, using the default version '${default_version}' (defined in $0)" >&2
  version="$default_version"
fi

version=$(echo "$version" | sed -e 's/^v//')

if [ "$short" -eq 1 ]; then
    # only leave two parts of the version (separated by the dots, any non-dot characters allowed)
    version=$(echo "$version" | sed 's/\([^.]\+\.[^.]\+\).*$/\1/')
fi

echo "$version"


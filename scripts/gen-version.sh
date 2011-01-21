#!/bin/bash
#
# Determines the version:
# 1. If the variable VERSION_FILE is set, it's the first word of that file
# 2. If we are in a git repository:
# 2.a. if VERSION.OVERRIDE file exists at the top of the repository, it's first word is the version (unless it's empty)
# 2.b. otherwise, use git-describe to generate a version number, using the closest 'v[0-9]*' tag
# 2.c. "-dirty" is appended to the version if there are local changes
# 3. otherwise, if VERSION file exists in current directory, it's first word is the version (unless it's empty)
# 4. otherwise, use default_version variable (defined below) as the version
# 5. 'v' at the beginning of the version number is stripped

set -euo pipefail

default_version='0.0-internal-unknown'

lf='
'

##### Find the top of the repository

start_dir="$(pwd)"

if [ -n "$(/bin/bash -c 'echo $VERSION_FILE')" ]; then # If VERSION_FILE is set, use it
    unset repo_dir
    repo_available=0
    version_file="$VERSION_FILE"
elif repo_dir="$(git rev-parse --show-cdup 2> /dev/null)" && \
   repo_dir="${repo_dir:-.}" && \
   [ -d "$repo_dir/.git" ]; then

    repo_available=1
    version_file="${repo_dir}/VERSION.OVERRIDE"
else
    unset repo_dir
    repo_available=0
    version_file="${start_dir}/VERSION"
fi

#### Compute version

version=''

[ -f "${version_file}" ] && read version < ${version_file}
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
echo "$version"


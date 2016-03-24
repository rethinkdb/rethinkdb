#!/usr/bin/env bash

# A simple package manager for RethinkDB dependencies
#
# Each package is a shell script that can override these functions:
#
#  pkg_install: Build and install the package into $install_dir
#               If a build step is necessary, first copy the source from $src_dir into $install_dir/build
#
#  pkg_fetch: Fetch the package source into $src_dir
#             The fetch function must fetch the source into a temporary directory
#             And then move that temporary directory to $src_dir
#
#  pkg_install-include: Copy the include files to $install_dir/include
#
#  pkg_depends: List the other packages that the package depends on
#
#  $version: The version of the package
#
# This pkg.sh script is called by ./configure and support/build.mk
# The first argument is the function that should be called
# The second argument is usually the package to load
# This script first defines utility functions used by the packages
# Then it loads the given package
# Then it calls the given command

set -eu

unset DESTDIR

src_url_backup=""
src_url_sha1=""

# Print the version number of the package
pkg_version () {
    echo $version
}

# Generate shell commands that add the paths for this package to the environment
pkg_environment () {
    if test -d "$install_dir/include"; then
        echo "export CXXFLAGS=\"\${CXXFLAGS:-} -isystem $(niceabspath "$install_dir/include")\""
        echo "export   CFLAGS=\"\${CFLAGS:-}   -isystem $(niceabspath "$install_dir/include")\""
    fi
    if [[ -d "$install_dir/lib" ]]; then
        echo "export LDFLAGS=\"\${LDFLAGS:-} -L$(niceabspath "$install_dir/lib")\""
        echo "export LD_LIBRARY_PATH=\"$(niceabspath "$install_dir/lib")\${LD_LIBRARY_PATH:+:}\${LD_LIBRARY_PATH:-}\""
        echo "export DYLD_LIBRARY_PATH=\"$(niceabspath "$install_dir/lib")\${DYLD_LIBRARY_PATH:+:}\${DYLD_LIBRARY_PATH:-}\""
    fi
    test -d "$install_dir/bin" && echo "export PATH=\"$(niceabspath "$install_dir/bin"):\$PATH\"" || :
    local pcdir="$install_dir/lib/pkgconfig"
    if [[ -e "$pcdir" ]]; then
        echo "export PKG_CONFIG_PATH=\"$pcdir\${PKG_CONFIG_PATH:+:}\${PKG_CONFIG_PATH:-}\""
    fi
}

pkg_make_tmp_fetch_dir () {
    tmp_dir=$(mktemp -d "$src_dir.fetch-XXXXXXXX")
}

pkg_remove_tmp_fetch_dir () {
    rm -rf "$tmp_dir"
}

pkg_fetch_archive () {
    pkg_make_tmp_fetch_dir

    local archive="${src_url##*/}"
    set +e # turn off auto-fail so we can fall back to $src_url_backup
    error_text=$(geturl "$src_url" "$tmp_dir/$archive")
    local download_failed=$?
    if [[ $download_failed -ne "0" ]] && [[ $src_url_backup ]]; then
        error_text=$(geturl "$src_url_backup" "$tmp_dir/$archive")
        download_failed=$?
    fi
    set -e # reenable auto-fail
    if [[ $download_failed -ne "0" ]]; then
        if [[ ! -z $error_text ]]; then
            error_text=": $error_text"
        fi
        error "failed to download $pkg from $src_url$error_text"
    fi

    if [ $src_url_sha1 ] && [[ `getsha1  "$tmp_dir/$archive"` != "$src_url_sha1" ]]; then
        error "sha1 for $pkg was incorrect: `getsha1  "$tmp_dir/$archive"` vs. expected: $src_url_sha1"
    fi

    local ext
    case "$archive" in
        *.tgz)     ext=tgz;     in_dir "$tmp_dir" tar -xzf "$archive" ;;
        *.tar.gz)  ext=tar.gz;  in_dir "$tmp_dir" tar -xzf "$archive" ;;
        *.tar.bz2) ext=tar.bz2; in_dir "$tmp_dir" tar -xjf "$archive" ;;
        *.tar.xz)  ext=tar.xz;  in_dir "$tmp_dir" tar -xJf "$archive" ;;
        *.zip)     ext=zip;     in_dir "$tmp_dir" unzip    "$archive" ;;
        *) error "don't know how to extract $archive"
    esac

    set -- "$tmp_dir"/*/

    if [[ "$#" != 1 ]]; then
        error "invalid archive contents: $archive"
    fi

    test -e "$src_dir" && rm -rf "$src_dir"

    mv "$1" "$src_dir"

    pkg_remove_tmp_fetch_dir
}

pkg_fetch_git () {
    pkg_make_tmp_fetch_dir

    git_clone_tag "$src_git_repo" "${src_git_ref:-$version}" "$tmp_dir"

    pkg_move_tmp_to_src
}

pkg_fetch () {
    if test -n "${src_url}"; then
        pkg_fetch_archive
    else
        error "fetch command for $pkg is broken. \$src_url should be defined"
    fi
}

pkg_move_tmp_to_src () {
    test -e "$src_dir" && rm -rf "$src_dir"
    mv "$tmp_dir" "$src_dir"
}

pkg_copy_src_to_build () {
    mkdir -p "$build_dir"
    cp -af "$src_dir/." "$build_dir"
}

pkg_install-include () {
    test -e "$install_dir/include" && rm -rf "$install_dir/include"
    if [[ -e "$src_dir/include" ]]; then
        mkdir -p "$install_dir/include"
        cp -RL "$src_dir/include/." "$install_dir/include"
    fi
}

pkg_configure () {
    in_dir "$build_dir" ./configure --prefix="$(niceabspath "$install_dir")" "$@"
}

pkg_make () {
    in_dir "$build_dir" make "$@"
}

pkg_install () {
    if ! fetched; then
        error "cannot install package, it has not been fetched"
    fi
    pkg_copy_src_to_build
    pkg_configure ${configure_flags:-}
    pkg_make install
}

pkg_depends () {
    true
}

pkg_depends_env () {
    for dep in $(pkg_depends); do
        eval "$(pkg environment $dep)"
    done
}

pkg_link-flags () {
    local lib="$install_dir/lib/lib$(lc $1).a"
    if [[ ! -e "$lib" ]]; then
        echo "pkg.sh: error: static library was not built: $lib" >&2
        exit 1
    fi
    echo "$lib"
}

cross_build_env () {
    # Unsetting these variables will pick up the toolchain from PATH.
    # Assuming that cross-compilation is achieved by setting these variables,
    # the toolchain in PATH will build executables that can be executed in the
    # build environment.
    unset CXX
    unset AR
    unset RANLIB
    unset CC
    unset LD
}

error () {
    echo "$0: ${pkg:-unknown}: $*" >&2
    exit 1
}

# Include a file local to $pkg_dir
include () {
    local inc="$1"
    shift
    . "$pkg_dir/$inc" "$@"
}

# Utility function copied from the configure script
niceabspath () {
    if [[ -d "$1" ]]; then
        (cd "$1" && pwd) && return
    fi
    local dir
    dir=$(dirname "$1")
    if [[ -d "$dir" ]] && dir=$(cd "$dir" && pwd); then
        echo "$dir/$(basename "$1")" | sed 's|^//|/|'
        return
    fi
    if [[ "${1:0:1}" = / ]]; then
        echo "$1"
    else
        echo "$(pwd)/$1"
    fi
}

# in_dir <dir> <cmd> <args...>
# Run the command in dir
in_dir () {
    local dir="$1"
    shift
    ( cd "$dir" && "$@" )
}

# Load a package and set related variables
load_pkg () {
    pkg=${1%%_*}
    local requested_version="${1#*_}"
    include "$pkg.sh"

    if [[ "$requested_version" != "$1" ]] && [[ "$requested_version" != "$version" ]]; then
        echo "Error: Version mismatch. Asked for ${pkg}_$requested_version but only $version is available" >&2
        echo "Error: Please re-run ./configure" >&2
        exit 1
    fi

    src_dir=$(niceabspath "$external_dir/$pkg""_$version")
    install_dir=$(niceabspath "$root_build_dir/external/$pkg""_$version")
    build_dir=$(niceabspath "$install_dir/build")
}

contains () {
    local d=" $1 "
    [ "${d% $2 *}" != "$d" ]
}

will_fetch () {
    contains "$FETCH_LIST" "$1"
}

# Test if the package has already been fetched
fetched () {
    test -e "$src_dir"
}

# Make a shallow clone of a specific git tag
git_clone_tag () {
    local remote tag repo
    remote=$1
    tag=$2
    repo=$3
    ( cd "$repo"
      unset GIT_DIR
      git init
      git remote add origin "$remote"
      git fetch --depth 1 origin "refs/tags/$tag"
      git checkout FETCH_HEAD
      rm -rf .git
    )
}

# Download a file
geturl () {
    if [[ -n "${CURL:-}" ]]; then
        $CURL --silent -S --fail --location "$1" -o "$2"
    else
        ${WGET:-wget} --quiet --output-document="$2" "$1" 
    fi
}

getsha1 () {
    if hash openssl 1>/dev/null 2>/dev/null; then
        openssl sha1 "$@" | awk '{print $NF}'
    elif hash sha1sum 1>/dev/null 2>/dev/null; then
        sha1sum "$@" | awk '{print $1}'
    elif hash shasum 1>/dev/null 2>/dev/null; then
        shasum -a 1 "$@" | awk '{print $NF}'
    elif hash sha1 1>/dev/null 2>/dev/null; then
        sha1 -q "$@"
    else
        error "Unable to get the sha1 checksum of $pkg, please install one of these tools: openssl, sha1sum, shasum, sha1"
    fi
}

# lowercase
lc () { echo "$*" | tr '[:upper:]' '[:lower:]'; }

pkg_script=$(niceabspath "$0")

pkg () {
    $pkg_script "$@"
}

# Configure some default paths
pkg_dir=$(niceabspath "$(dirname $0)")
root_dir=$(niceabspath "$pkg_dir/../../..")
conf_dir=$(niceabspath "$pkg_dir/../config")
external_dir=$(niceabspath "$pkg_dir/../../../external")
root_build_dir=${BUILD_ROOT_DIR:-$(niceabspath "$pkg_dir/../../../build")}

# These variables should be passed to this script from support/build.mk
WGET=${WGET:-}
CURL=${CURL:-}
OS=${OS:-}
# FETCH_LIST
# NPM
# BUILD_ROOT_DIR
# PTHREAD_LIBS

# Read the command
cmd=$1
shift

# Load the package
load_pkg "$1"
shift

# Trace commands
if [[ "${TRACE:-}" = 1 ]]; then
    set -x
fi

# Prepare the environment
pkg_depends_env

# Run the command
pkg_"$cmd" "$@"

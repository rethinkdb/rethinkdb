#!/bin/bash
# Copyright 2026 RethinkDB, all rights reserved.
#
# Package an existing Linux build as a DEB, or prepare a Debian source package.
# Binary mode never configures or compiles. Source mode only prepares the
# source tree and runs debuild -S. Debian tooling compiles that source package.

set -euo pipefail

# shellcheck source=cmake-system-name.sh
source "$(
    unset CDPATH
    cd -- "$(dirname -- "$0")" && pwd
)/cmake-system-name.sh"

mode="${MODE:-binary}"
source_dir="${SOURCE_DIR:-}"
binary_dir="${BINARY_DIR:-}"

if [[ -z "${source_dir}" ]]; then
    source_dir=$(
        unset CDPATH
        cd -- "$(dirname -- "$0")/.." && pwd
    )
fi
if [[ -z "${binary_dir}" ]]; then
    printf 'BINARY_DIR is required\n' >&2
    exit 1
fi

require_linux_build() {
    local cache="${binary_dir}/CMakeCache.txt"
    local server="${binary_dir}/rethinkdb"
    local system_name=""

    if [[ ! -f "${cache}" ]]; then
        printf 'Not a CMake build directory: %s\n' "${binary_dir}" >&2
        exit 1
    fi
    system_name=$(cmake_system_name "${binary_dir}" || true)
    if [[ "${system_name}" != "Linux" ]]; then
        printf 'DEB packages require a Linux CMake build: %s\n' "${binary_dir}" >&2
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
}

package_binary() {
    local cpack_cmd="${CPACK:-cpack}"
    local output="${binary_dir}/packages/deb"
    local -a cpack_args

    if ! command -v file >/dev/null 2>&1; then
        printf 'DEB packages require the file utility\n' >&2
        exit 1
    fi
    require_linux_build
    rm -rf "${output}"
    mkdir -p "${output}"
    cpack_args=(
        -G DEB
        -B "${output}"
        --config "${binary_dir}/CPackConfig.cmake"
    )
    if [[ -n "${CONFIG:-}" ]]; then
        cpack_args+=(-C "${CONFIG}")
    fi
    "${cpack_cmd}" "${cpack_args[@]}"
}

package_source() {
    local version="${RETHINKDB_VERSION:?}"
    local ubuntu_release="${UBUNTU_RELEASE:-}"
    local deb_release="${DEB_RELEASE:-}"
    local build_number="${PACKAGE_BUILD_NUMBER:-1}"
    local os_release
    local deb_version
    local pkg_dir
    local build_depends
    local -a sign_args

    if [[ -n "${deb_release}" ]]; then
        os_release="${deb_release}"
    elif [[ -n "${ubuntu_release}" ]]; then
        os_release="${ubuntu_release}"
    else
        os_release="unstable"
    fi

    deb_version="${version//-/\+}~${build_number}${ubuntu_release}${deb_release}"
    pkg_dir="${binary_dir}/packages/dsc/rethinkdb-${version}"
    sign_args=(-us -uc)
    if [[ "${SIGN_PACKAGE:-0}" == 1 ]]; then
        sign_args=()
    fi

    if [[ ! -d "${binary_dir}/packages/rethinkdb-${version}" ]]; then
        printf 'dist tree is missing: %s\n' \
            "${binary_dir}/packages/rethinkdb-${version}" >&2
        exit 1
    fi

    rm -rf "${binary_dir}/packages/dsc"
    mkdir -p "${binary_dir}/packages/dsc"
    cp -a "${binary_dir}/packages/rethinkdb-${version}" "${pkg_dir}"
    cp -a "${source_dir}/packaging/debian" "${pkg_dir}/debian"

    PRODUCT_NAME=rethinkdb \
    PRODUCT_VERSION="${deb_version}" \
    OS_RELEASE="${os_release}" \
        "${source_dir}/scripts/gen-changelog.sh" > "${pkg_dir}/debian/changelog"
    printf '%s\n' "${deb_version}" > "${pkg_dir}/debian/rethinkdb.version"

    build_depends="m4, debhelper, fakeroot, libcurl4-openssl-dev, g++, python3, python-is-python3, libssl-dev, cmake"
    if [[ "${BUILD_PORTABLE:-0}" != 1 ]]; then
        build_depends+=", protobuf-compiler, libprotobuf-dev"
    fi

    m4 \
        -D PACKAGE_NAME=rethinkdb \
        -D "PACKAGE_VERSION=${deb_version}" \
        -D "DEB_BUILD_DEPENDS=${build_depends}" \
        -D VERSIONED_QUALIFIED_PACKAGE_NAME=rethinkdb \
        "${pkg_dir}/debian/control.in" > "${pkg_dir}/debian/control"
    rm -f "${pkg_dir}/debian/control.in"

    (
        cd "${pkg_dir}"
        debuild -S -sa "${sign_args[@]}"
    )
}

case "${mode}" in
    binary)
        package_binary
        ;;
    src)
        package_source
        ;;
    *)
        printf 'MODE must be binary or src, not %s\n' "${mode}" >&2
        exit 1
        ;;
esac

#!/bin/bash
# Copyright 2026 RethinkDB, all rights reserved.
#
# Build a Debian source package, or a binary package, from the CMake dist tree.
set -euo pipefail

version="${RETHINKDB_VERSION:?}"
source_dir="${SOURCE_DIR:?}"
binary_dir="${BINARY_DIR:?}"
mode="${MODE:-src}"
ubuntu_release="${UBUNTU_RELEASE:-}"
deb_release="${DEB_RELEASE:-}"
build_number="${PACKAGE_BUILD_NUMBER:-1}"

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
    printf 'dist tree is missing: %s\n' "${binary_dir}/packages/rethinkdb-${version}" >&2
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

build_depends="m4, debhelper, fakeroot, libncurses-dev, libcurl4-openssl-dev, g++, python3, python-is-python3, libssl-dev, cmake"
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

if [[ "${mode}" == binary ]]; then
    jobs=$(sh "${source_dir}/cmake/parallel-jobs.sh")
    if [[ ! "${jobs}" =~ ^[1-9][0-9]*$ ]]; then
        printf 'parallel-jobs.sh returned %s\n' "${jobs}" >&2
        exit 1
    fi
    printf 'Using %s compile jobs\n' "${jobs}"
    (
        cd "${pkg_dir}"
        dpkg-buildpackage --jobs="${jobs}" -rfakeroot "${sign_args[@]}"
    )
else
    (
        cd "${pkg_dir}"
        debuild -S -sa "${sign_args[@]}"
    )
fi

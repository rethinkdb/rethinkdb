#!/bin/bash

# This script is used to build RethinkDB RPM on CentOS 6.4 and 7
#
# It requires:
# * The build dependencies: https://www.rethinkdb.com/docs/install/centos/
# * gem install fpm

set -eu

main () {
    # Release compiles of Protobuf and the server use several GiB per cc1plus.
    # An uncapped --parallel, or the old default of -j 8, is OOM-killed on
    # smaller Rocky/RHEL machines. ExternalProject gmake reads MAKEFLAGS.
    jobs=$(sh cmake/parallel-jobs.sh)
    if [[ ! "${jobs}" =~ ^[1-9][0-9]*$ ]]; then
        printf 'parallel-jobs.sh returned %s\n' "${jobs}" >&2
        exit 1
    fi
    printf 'Using %s compile jobs\n' "${jobs}"
    export MAKEFLAGS="-j${jobs}"
    ARCH=`gcc -dumpmachine | cut -f 1 -d -`
    RPM_ROOT=build/packages/rpm
    VERSION=`./scripts/gen-version.sh | sed -e s/-/_/g`
    RPM_PACKAGE=build/packages/rethinkdb-$VERSION.$ARCH.rpm
    SYMBOLS_FILE_IN=build/release/rethinkdb.debug
    SYMBOLS_FILE_OUT=$RPM_PACKAGE.debug.bz2
    DESCRIPTION='RethinkDB is built to store JSON documents, and scale to multiple servers with very little effort. It has a pleasant query language that supports really useful queries like table joins and group by.'
    tmpfile BEFORE_INSTALL <<EOF
getent group rethinkdb >/dev/null || groupadd -r rethinkdb
getent passwd rethinkdb >/dev/null || \
    useradd --system --no-create-home --gid rethinkdb --shell /sbin/nologin \
    --comment "RethinkDB Daemon" rethinkdb
EOF

    if [[ -z "${NOCONFIGURE:-}" ]]; then
        cmake -S . -B build/release \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=/usr \
            -DCMAKE_INSTALL_SYSCONFDIR=/etc \
            -DRETHINKDB_LOCALSTATEDIR=/var \
            -DRETHINKDB_FETCH_JEMALLOC=ON \
            -DRETHINKDB_FETCH_BOOST=ON \
            -DRETHINKDB_FETCH_RE2=ON \
            -DRETHINKDB_FETCH_QUICKJS=ON \
            -DRETHINKDB_FETCH_PROTOBUF=ON \
            -DRETHINKDB_BUILD_PORTABLE=ON \
            -DRETHINKDB_ALLOW_WARNINGS=ON \
            -DRETHINKDB_SPLIT_SYMBOLS=ON \
            -DRETHINKDB_ALLOCATOR=jemalloc
    fi

    cmake --build build/release --parallel "${jobs}"
    DESTDIR="$RPM_ROOT" cmake --install build/release

    ... () { command="$command $(for x in "$@"; do printf "%q " "$x"; done)"; }

    GLIBC_VERSION=`rpm -qa --queryformat '%{VERSION}\n' glibc | head -n1`
    LIBSTDCPP_VERSION=`rpm -qa --queryformat '%{VERSION}\n' libstdc++ | head -n1`
    LIBCURL_VERSION=`rpm -qa --queryformat '%{VERSION}\n' libcurl | head -n1`
    OPENSSL_VERSION=`rpm -qa --queryformat '%{VERSION}\n' openssl-libs | head -n1`
    ZLIB_VERSION=`rpm -qa --queryformat '%{VERSION}\n' zlib | head -n1`

    command=fpm
    ... -t rpm                  # Build an RPM package
    ... --package $RPM_PACKAGE
    ... --name rethinkdb
    ... --license 'ASL 2.0'
    ... --vendor RethinkDB
    ... --category Database
    ... --version "$VERSION"
    ... --iteration "`./scripts/gen-version.sh -r`"
    ... --depends "glibc >= $GLIBC_VERSION"
    ... --depends "libstdc++ >= $LIBSTDCPP_VERSION"
    ... --depends "libcurl >= $LIBCURL_VERSION"
    ... --depends "openssl-libs >= $OPENSSL_VERSION"
    ... --depends "zlib >= $ZLIB_VERSION"
    ... --conflicts 'rethinkdb'
    ... --architecture "$ARCH"
    ... --maintainer 'RethinkDB <devops@rethinkdb.com>'
    ... --description "$DESCRIPTION"
    ... --url 'https://www.rethinkdb.com/'
    ... --before-install "$BEFORE_INSTALL"
    ... -s dir -C $RPM_ROOT     # Directory containing the installed files
    ... usr etc var             # Directories to package in the package
    eval $command

    bzip2 -c "$SYMBOLS_FILE_IN" > "$SYMBOLS_FILE_OUT"
}

tmpfile () {
    local _file=`mktemp`
    cat >"$_file"
    at_exit rm "$_file"
    eval "$1=$(printf %q "$_file")"
}

at_exit () {
    local cmd=
    for x in "$@"; do
        cmd="$cmd $(printf %q "$x")"
    done
    AT_EXIT_ALL=${AT_EXIT_ALL:-}'
'"$cmd"
    trap exit_handler EXIT
}

exit_handler () {
    eval "$AT_EXIT_ALL"
}

main "$@"

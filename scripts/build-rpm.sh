#!/bin/bash

# This script is used to build RethinkDB RPM on CentOS 6.4 and 7
#
# It requires:
# * The build dependencies: https://www.rethinkdb.com/docs/install/centos/
# * gem install fpm

set -eu

main () {
    MAKEFLAGS=${MAKEFLAGS:- -j 8}
    export MAKEFLAGS
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

    test -n "${NOCONFIGURE:-}" || ./configure --static all --fetch all --prefix=/usr --sysconfdir=/etc --localstatedir=/var

    `make command-line` install DESTDIR=$RPM_ROOT BUILD_PORTABLE=1 ALLOW_WARNINGS=1 SPLIT_SYMBOLS=1

    ... () { command="$command $(for x in "$@"; do printf "%q " "$x"; done)"; }

    GLIBC_VERSION=`rpm -qa --queryformat '%{VERSION}' glibc`

    command=fpm
    ... -t rpm                  # Build an RPM package
    ... --package $RPM_PACKAGE
    ... --name rethinkdb
    ... --license AGPL
    ... --vendor RethinkDB
    ... --category Database
    ... --version "$VERSION"
    ... --iteration "`./scripts/gen-version.sh -r`"
    ... --depends "glibc >= $GLIBC_VERSION"
    ... --conflicts 'rethinkdb'
    ... --architecture "$ARCH"
    ... --maintainer 'RethinkDB <devops@rethinkdb.com>'
    ... --description "$DESCRIPTION"
    ... --url 'http://www.rethinkdb.com/'
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

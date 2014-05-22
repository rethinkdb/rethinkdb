#!/bin/bash

# This script is used to build RethinkDB RPM on CentOS 6.4
#
# It requires:
# yum --assumeyes install http://dl.fedoraproject.org/pub/epel/6/i386/epel-release-6-8.noarch.rpm
# yum --assumeyes install git-core boost-static m4 gcc-c++ python-pip v8-devel nodejs npm ncurses-devel which make PyYAML subversion ncurses-static rubygems ruby-devel build-rpm rpm-build zlib-devel zlib-static
# gem install fpm

set -eu

main () {
    MAKEFLAGS=${MAKEFLAGS:- -j 8}
    export MAKEFLAGS
    ARCH=`gcc -dumpmachine | cut -f 1 -d -`
    RPM_ROOT=build/packages/rpm
    VERSION=`./scripts/gen-version.sh | sed -e s/-/_/g`
    RPM_PACKAGE=build/packages/rethinkdb-$VERSION.$ARCH.rpm
    DESCRIPTION='RethinkDB is built to store JSON documents, and scale to multiple machines with very little effort. It has a pleasant query language that supports really useful queries like table joins and group by.'
    tmpfile BEFORE_INSTALL <<EOF
getent group rethinkdb >/dev/null || groupadd -r rethinkdb
getent passwd rethinkdb >/dev/null || \
    useradd --system --no-create-home --gid rethinkdb --shell /sbin/nologin \
    --comment "RethinkDB Daemon" rethinkdb
EOF

    test -n "${NOCONFIGURE:-}" || ./configure --static all --fetch all --prefix=/usr --sysconfdir=/etc --localstatedir=/var

    `make command-line` install DESTDIR=$RPM_ROOT BUILD_PORTABLE=1 ALLOW_WARNINGS=1 SPLIT_SYMBOLS=1

    ... () { command="$command $(for x in "$@"; do printf "%q " "$x"; done)"; }

    command=fpm
    ... -t rpm                  # Build an RPM package
    ... --package $RPM_PACKAGE
    ... --name rethinkdb
    ... --license AGPL
    ... --vendor RethinkDB
    ... --category Database
    ... --version "$VERSION"
    ... --iteration "`./scripts/gen-version.sh -r`"
    ... --depends 'glibc >= 2.12' # Only if you build with glibc 2.12
    ... --conflicts 'rethinkdb'
    ... --architecture "$ARCH"
    ... --maintainer 'RethinkDB <devops@rethinkdb.com>'
    ... --description "$DESCRIPTION"
    ... --url 'http://www.rethinkdb.com/'
    ... --before-install "$BEFORE_INSTALL"
    ... -s dir -C $RPM_ROOT     # Directory containing the installed files
    ... usr etc var             # Directories to package in the package
    eval $command
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

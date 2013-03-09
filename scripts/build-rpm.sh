#!/bin/bash

set -eu

RPM_ROOT=build/packages/rpm
VERSION=`./scripts/gen-version.sh | sed -e s/-/_/g`
RPM_PACKAGE=build/packages/rethinkdb-$VERSION.rpm
DESCRIPTION='RethinkDB is built to store JSON documents, and scale to multiple machines with very little effort. It has a pleasant query language that supports really useful queries like table joins and group by.'

./configure --static all --allow-fetch --prefix=/usr --sysconfdir=/etc --localstatedir=/var

make install-binaries install-web DESTDIR=$RPM_ROOT BUILD_PORTABLE=1

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
... --depends 'libaio >= 0.3.106'
... --depends 'glibc >= 2.5'
### --architecture TODO
... --maintainer 'RethinkDB <devops@rethinkdb.com>'
... --description "$DESCRIPTION"
... --url 'http://www.rethinkdb.com/'
... -s dir -C $RPM_ROOT     # Directory containing the installed files
... usr                     # Directories to package in the package
eval $command
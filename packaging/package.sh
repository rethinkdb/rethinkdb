#!/bin/bash
# Copyright 2010-2012 RethinkDB, all rights reserved.


echo "THIS SCRIPT IS UNFINISHED!!!"
exit 1

NAME=rethinkdb
SUMMARY="RethinkDB - the database for solid drives"
COMPANY_NAME="Hexagram 49, Inc."
DESCRIPTION="$SUMMARY" # TODO
RELEASE=1
LICENSE="RethinkDB Beta Test License 0.1"
RPM_GROUP="Development/Tools"

PRIVIDES="" # TODO
REQUIRES="" # TODO

REPO_TOP_DIR="$(dirname $0)/.."
BUILD_DIR="${REPO_TOP_DIR}/build/release"
RPM_PACKAGE_TOP_DIR="${REPO_TOP_DIR}/build/packages/rpm"
BROOTPATH="${RPM_PACKAGE_TOP_DIR}/BUILD"
BINARIES="${BUILD_DIR}/rethinkdb"
DOCS=""
MAN_PAGES="${BUILD_DIR}/docs/rethinkdb.1"

ALL_FILES="$BINARIES $DOCS $MAN_PAGES"

echo $0: $REPO_TOP_DIR
exit 0

set -eou pipefail

get_version() {
}

build_rpm() {
  echo Building RPM...

  VERSION="$(get_version)"
  SPEC_PATH="$(mktemp -t XXXX.spec)"
  cat > "$SPEC_PATH" << EOF
Summary:   $SUMMARY
Name:      $NAME
Version:   $VERSION
Release:   $RELEASE
License:   $LICENSE
Packager:  $COMPANY_NAME
Group:     $RPM_GROUP
BuildRoot: $BROOTPATH
Provides:  $PROVIDES
Requires:  $REQUIRES,/bin/sh

%description
$DESCRIPTION

%pre

%post

%preun

%postun

%files

EOF

  rpmbuild -
}

build_deb() {
  echo Building DEB...
}

#if [ -z "$1" ]; then
#  echo "Usage: $0 <rpm|deb>" >&2
#else
  case "$1" in
    rpm) build_rpm ;;
    deb) build_deb ;;
    *) echo "Usage: $0 <rpm|deb>" >&2 ;;
  esac
#fi

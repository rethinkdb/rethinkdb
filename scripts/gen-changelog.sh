#!/bin/sh
# Copyright 2010-2012 RethinkDB, all rights reserved.
# This script is for generating a changelog file for the Debian source package. git-dch is problematic, so we generate a stub changelog each time.
# The script omits automatic base-finding and presently expects to be run from the root of the repository.
# Variables expected in the environment include
# 	UBUNTU_RELEASE
#	DEB_RELEASE
#	DEB_RELEASE_NUM
#	VERSIONED_QUALIFIED_PACKAGE_NAME
# 	PACKAGE_VERSION
# .

PRODBASE=. ;
PRODUCT_NAME="$VERSIONED_QUALIFIED_PACKAGE_NAME" ;
# PRODUCTVERSION="`"$PRODBASE"/scripts/gen-version.sh`""-0"
PRODUCT_VERSION="$PACKAGE_VERSION""-0"
if [ "$UBUNTU_RELEASE" != "" ] ;
then
	# For Ubuntu, we use Debian number 0 and append the Ubuntu stuff.
	PRODUCT_VERSION="$PACKAGE_VERSION"-0ubuntu1~"$UBUNTU_RELEASE" ;
	OS_RELEASE="$UBUNTU_RELEASE" ;
elif [ "$DEB_RELEASE" != "" ] ;
then
	# For Debian, we choose a Debian number corresponding to the Debian release.
	PRODUCT_VERSION="$PACKAGE_VERSION"-"$DEB_RELEASE_NUM" ;
	OS_RELEASE="$DEB_RELEASE" ;
else
	OS_RELEASE="unstable" ;
fi ;
TIMESTAMP_TIME="`date "+%a, %d %b %Y %H:%M:%S"`" ;
TIMESTAMP_OFFSET="-0800" ;
TIMESTAMP_FULL="$TIMESTAMP_TIME"" ""$TIMESTAMP_OFFSET" ;
AGENT_NAME="RethinkDB Packaging"
AGENT_MAIL="packaging@rethinkdb.com"
CHANGELOG_FILE="$PRODBASE"/"debian/changelog" ;
echo "$PRODUCT_NAME"" (""$PRODUCT_VERSION"") ""$OS_RELEASE""; urgency=low" > "$CHANGELOG_FILE" ;
echo "" >> "$CHANGELOG_FILE" ;
echo "  * Release." >> "$CHANGELOG_FILE" ;
echo "" >> "$CHANGELOG_FILE" ;
echo " -- ""$AGENT_NAME"" <""$AGENT_MAIL"">  ""$TIMESTAMP_FULL" >> "$CHANGELOG_FILE" ;
# Note that there are two spaces between the e-mail address and the time-stamp. This was no accident.


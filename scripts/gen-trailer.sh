#!/bin/sh
# Copyright 2010-2012 RethinkDB, all rights reserved.
# This script is for generating a version trailer for Ubuntu and Debian packages.
# Variables taken from the environment include
# 	UBUNTU_RELEASE
#	DEB_RELEASE
#	DEB_RELEASE_NUM
#	VERSIONED_PACKAGE_NAME
# 	PACKAGE_VERSION
# . These can be blank as desired.

# We can automatically fill in blanks for Debian things if necessary. The sed blocks are copied from the Makefile.
if [ "$DEB_RELEASE" == "" ] && [ "$DEB_RELEASE_NUM" != "" ] ;
then
	DEB_RELEASE="`echo "$DEB_RELEASE_NUM" | sed -e 's/^1$/buzz/g' -e 's/^2$/rex/g' -e 's/^3$/bo/g' -e 's/^4$/hamm/g' -e 's/^5$/slink/g' -e 's/^6$/potato/g' -e 's/^7$/woody/g' -e 's/^8$/sarge/g' -e 's/^9$/etch/g' -e 's/^10$/lenny/g' -e 's/^11$/squeeze/g' -e 's/^12$/wheezy/g' -e 's/^13$/jessie/g' | grep '^[a-z]*$'`"
elif [ "$DEB_RELEASE" != "" ] && [ "$DEB_RELEASE_NUM" == "" ] ;
	DEB_RELEASE_NUM="`echo "$DEB_RELEASE" | sed -e 's/^buzz$/1/g' -e 's/^rex$/2/g' -e 's/^bo$/3/g' -e 's/^hamm$/4/g' -e 's/^slink$/5/g' -e 's/^potato$/6/g' -e 's/^woody$/7/g' -e 's/^sarge$/8/g' -e 's/^etch$/9/g' -e 's/^lenny$/10/g' -e 's/^squeeze$/11/g' -e 's/^wheezy$/12/g' -e 's/^jessie$/13/g' | grep '^[0-9]*$'`"
fi ;

# The following block is pasted from gen-changelog.sh. We ought to keep these from diverging.
if [ "$UBUNTU_RELEASE" != "" ] ;
then
	# For Ubuntu, we use Debian number 0 and append the Ubuntu stuff.
	PRODUCT_VERSION="$PACKAGE_VERSION"-0ubuntu1~"$UBUNTU_RELEASE" ;
	OS_RELEASE="$UBUNTU_RELEASE" ;
elif [ "$DEB_RELEASE" != "" ] ;
	# For Debian, we choose a Debian number corresponding to the Debian release.
	PRODUCT_VERSION="$PACKAGE_VERSION"-"$DEB_RELEASE_NUM" ;
	OS_RELEASE="$DEB_RELEASE" ;
else
	OS_RELEASE="unstable" ;
fi ;
echo "$PRODUCT_VERSION"


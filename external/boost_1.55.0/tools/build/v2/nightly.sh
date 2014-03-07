#!/bin/bash

# Copyright 2004, 2005, 2006 Vladimir Prus 
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 

# This script create a nightly tarball of Boost.Build V2
# and updates the web site.

# Create the packages
set -e
trap "echo 'Nightly build failed'" ERR

export QTDIR=/usr/share/qt3
export LC_ALL=C
export LC_MESSAGES=C
export LANG=C
cd /tmp
rm -rf boost-build-nightly
mkdir boost-build-nightly
echo "Checking out sources"
svn co http://svn.boost.org/svn/boost/trunk/tools/build/v2 boost-build-nightly/boost-build > /tmp/boost_build_checkout_log
mv /tmp/boost_build_checkout_log boost-build-nightly/checkout-log
cd boost-build-nightly/boost-build/
echo "Building packages and uploading docs"
./roll.sh > ../roll-log 2>&1
cd ..
echo "Uploading packages"
scp boost-build.zip boost-build.tar.bz2 vladimir_prus,boost@web.sourceforge.net:/home/groups/b/bo/boost/htdocs/boost-build2 > scp-log
echo "Nightly build successful"

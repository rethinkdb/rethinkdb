#!/usr/bin/env bash

# Build a branches/release snapshot for Posix, using LF line termination

# © Copyright 2008 Beman Dawes
# Distributed under the Boost Software License, Version 1.0.
# See http://www.boost.org/LICENSE_1_0.txt

echo "Build a branches/release snapshot for POSIX, using LF line termination..."

echo "Removing old files..."
rm -r -f posix
rm -r -f svn_info

echo "Exporting files from subversion..."
# leave an audit trail, which is used by inspect to determine revision number
#   use --non-recursive rather than --depth=files until the linux distros catch up
svn co --non-recursive http://svn.boost.org/svn/boost/branches/release svn_info
svn export --non-interactive --native-eol LF http://svn.boost.org/svn/boost/branches/release posix

#echo "Building bjam..."
# failure to use an up-to-date copy of bjam has caused much wasted effort.
#pushd posix/tools/build/v2/engine
#./build.sh gcc
#popd
#
#echo "Building docs..."
#pushd posix/doc
#../tools/build/v2/engine/bin.cygwinx86/bjam --toolset=gcc &>../../posix-bjam.log
#popd

echo "Cleaning up and renaming..."
#rm -r posix/bin.v2
SNAPSHOT_DATE=`eval date +%Y-%m-%d`
echo SNAPSHOT_DATE is $SNAPSHOT_DATE
mv posix boost-posix-$SNAPSHOT_DATE
rm -f posix.tar.gz
rm -f posix.tar.bz2

echo "Building .gz file..."
tar cfz posix.tar.gz boost-posix-$SNAPSHOT_DATE
echo "Building .bz2 file..."
gunzip -c posix.tar.gz | bzip2 >posix.tar.bz2
mv boost-posix-$SNAPSHOT_DATE posix

echo "Creating ftp script..."
echo "dir" >posix.ftp
echo "binary" >>posix.ftp

#echo "put posix.tar.gz" >>posix.ftp
#echo "mdelete boost-posix*.gz" >>posix.ftp
#echo "rename posix.tar.gz boost-posix-$SNAPSHOT_DATE.tar.gz" >>posix.ftp

echo "put posix.tar.bz2" >>posix.ftp
echo "mdelete boost-posix*.bz2" >>posix.ftp
echo "rename posix.tar.bz2 boost-posix-$SNAPSHOT_DATE.tar.bz2" >>posix.ftp

echo "dir" >>posix.ftp
echo "bye" >>posix.ftp

echo "Running ftp script..."
# use cygwin ftp rather than Windows ftp
/usr/bin/ftp -v -i boost.cowic.de <posix.ftp

echo POSIX snapshot complete!

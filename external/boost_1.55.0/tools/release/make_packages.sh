#!/bin/sh

# Build branches/release packages

# © Copyright 2008 Beman Dawes
# Distributed under the Boost Software License, Version 1.0. See http://www.boost.org/LICENSE_1_0.txt

if [ $# -lt 1 ]
then
 echo "invoke:" $0 "package-name"
 echo "example:" $0 "boost_1_35_0_RC3"
 exit 1
fi

echo "preping posix..."
rm -r posix/bin.v2 2>/dev/null
rm -r posix/dist 2>/dev/null
mv posix $1
rm -f $1.tar.gz 2>/dev/null
rm -f $1.tar.bz2 2>/dev/null
echo "creating gz..."
tar cfz $1.tar.gz $1
echo "creating bz2..."
gunzip -c $1.tar.gz | bzip2 >$1.tar.bz2
echo "cleaning up..."
mv $1 posix

echo "preping windows..."
rm -r windows/bin.v2 2>/dev/null
rm -r windows/dist 2>/dev/null
mv windows $1
rm -f $1.zip 2>/dev/null
rm -f $1.7z 2>/dev/null
echo "creating zip..."
zip -r $1.zip $1
echo "creating 7z..."
7z a -r $1.7z $1
echo "cleaning up..."
mv $1 windows

echo "done automatic processing; you must now upload packages manually"
exit 0




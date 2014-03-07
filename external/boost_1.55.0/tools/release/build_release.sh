#!/usr/bin/env bash

# Build release packages

# Copyright 2008 Beman Dawes
# Distributed under the Boost Software License, Version 1.0. See http://www.boost.org/LICENSE_1_0.txt

if [ $# -lt 1 ]
then
 echo "invoke:" $0 "release-name"
 echo "example:" $0 "boost_1_35_0_RC3"
 exit 1
fi

./load_posix.sh
./load_windows.sh
./build_docs.sh posix
./build_docs.sh windows
./build_release_packages.sh $1


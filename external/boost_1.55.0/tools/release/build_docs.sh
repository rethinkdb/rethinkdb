#!/usr/bin/env bash

# Build docs

# Copyright 2008 Beman Dawes
# Distributed under the Boost Software License, Version 1.0. See http://www.boost.org/LICENSE_1_0.txt

if [ $# -lt 1 ]
then
 echo "invoke:" $0 "directory-name"
 echo "example:" $0 "posix"
 exit 1
fi

echo building $1 docs...
pushd $1/doc
bjam --v2 >../../$1-bjam.log
ls html
popd


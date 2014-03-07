#!/bin/sh -e

# Copyright 2009 Daniel James.
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

rm -r html

cd doxygen/gil_standalone/
doxygen gil_boost.doxygen
cd -

cd html
../shorten_file_name.sh
cd -
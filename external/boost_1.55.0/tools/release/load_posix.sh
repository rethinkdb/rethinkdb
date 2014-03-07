#!/usr/bin/env bash

# Load posix directory from branches/release snapshot, using LF line termination

# Copyright 2008 Beman Dawes
# Distributed under the Boost Software License, Version 1.0. See http://www.boost.org/LICENSE_1_0.txt

rm -r -f posix 2>/dev/null
svn export --non-interactive --native-eol LF http://svn.boost.org/svn/boost/branches/release posix


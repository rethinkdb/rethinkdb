#!/usr/bin/env bash

# Load windows directory from branches/release snapshot, using CR/LF line termination

# Copyright 2008 Beman Dawes
# Distributed under the Boost Software License, Version 1.0. See http://www.boost.org/LICENSE_1_0.txt

rm -r -f windows 2>/dev/null
svn export --non-interactive --native-eol CRLF http://svn.boost.org/svn/boost/branches/release windows


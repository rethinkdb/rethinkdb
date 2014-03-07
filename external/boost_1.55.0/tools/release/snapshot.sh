#!/usr/bin/env bash

# © Copyright 2008 Beman Dawes
# Distributed under the Boost Software License, Version 1.0.
# See http://www.boost.org/LICENSE_1_0.txt

sleep 5s
echo Using $BOOST_TRUNK as boost trunk
date
$BOOST_TRUNK/tools/release/snapshot_posix.sh
date
$BOOST_TRUNK/tools/release/snapshot_windows.sh
date
$BOOST_TRUNK/tools/release/snapshot_inspect.sh
date
sleep 5s

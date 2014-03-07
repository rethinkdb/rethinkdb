#!/usr/bin/env bash

# Inspect snapshot

# © Copyright 2008 Beman Dawes
# Distributed under the Boost Software License, Version 1.0.
# See http://www.boost.org/LICENSE_1_0.txt

# This script uses ftp, and thus assumes ~/.netrc contains a machine ... entry

pushd posix/tools/inspect/build
bjam
popd
echo inspect...
pushd posix
dist/bin/inspect >../inspect.html
popd

# create the ftp script
echo create ftp script...
echo "dir" >inspect.ftp
echo "binary" >>inspect.ftp
echo "put inspect.html" >>inspect.ftp
echo "delete inspect-snapshot.html" >>inspect.ftp
echo "rename inspect.html inspect-snapshot.html" >>inspect.ftp
echo "dir" >>inspect.ftp
echo "bye" >>inspect.ftp
# use cygwin ftp rather than Windows ftp
echo ftp...
/usr/bin/ftp -v -i boost.cowic.de <inspect.ftp
echo inspect.sh complete

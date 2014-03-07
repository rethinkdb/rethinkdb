#! /bin/bash

# boost::hs installer
#
# Copyright 2006 Matias Capeletto
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# This script install the .xml kate syntax files in KDE

echo "Installing boost::hs"
echo "A few xml files will be copy to the place KDE store katepart sources."
echo "Files to install"

ls syntax/*.xml -1

echo "Installing..."

cp syntax/*.xml /usr/share/apps/katepart/syntax

echo "Done!"
echo ""


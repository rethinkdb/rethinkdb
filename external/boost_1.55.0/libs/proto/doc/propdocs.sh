#!/bin/sh

#  Copyright 2012 Eric Niebler
#
#  Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

rsync --delete --rsh=ssh --recursive -p html/* eric_niebler,boost-sandbox@web.sourceforge.net:/home/groups/b/bo/boost-sandbox/htdocs/libs/proto/doc/html

rsync --delete --rsh=ssh --recursive -p src/* eric_niebler,boost-sandbox@web.sourceforge.net:/home/groups/b/bo/boost-sandbox/htdocs/libs/proto/doc/src

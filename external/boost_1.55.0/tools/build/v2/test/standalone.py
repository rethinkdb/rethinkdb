#!/usr/bin/python

# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)


# Regression tests: standalone project were not able to refer to targets
# declared in themselves.

t.write("a.cpp", "int main() {}\n")
t.write("jamroot.jam", "import standalone ;")
t.write("standalone.jam", """\
import alias ;
import project ;

project.initialize $(__name__) ;
project standalone ;

local pwd = [ PWD ] ;

alias x : $(pwd)/../a.cpp ;
alias runtime : x ;
""")

t.write("standalone.py", """\
from b2.manager import get_manager

# FIXME: this is ugly as death
get_manager().projects().initialize(__name__)

import os ;

# This use of list as parameter is also ugly.
project(['standalone'])

pwd = os.getcwd()
alias('x', [os.path.join(pwd, '../a.cpp')])
alias('runtime', ['x'])
""")


t.write("sub/jamfile.jam", "stage bin : /standalone//runtime ;")

t.run_build_system(subdir="sub")
t.expect_addition("sub/bin/a.cpp")

t.cleanup()

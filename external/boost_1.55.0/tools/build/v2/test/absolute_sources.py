#!/usr/bin/python

# Copyright 2003, 2004 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test that sources with absolute names are handled OK.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("jamroot.jam", "path-constant TOP : . ;")
t.write("jamfile.jam", """\
local pwd = [ PWD ] ;
ECHO $(pwd) XXXXX ;
exe hello : $(pwd)/hello.cpp $(TOP)/empty.cpp ;
""")
t.write("hello.cpp", "int main() {}\n")
t.write("empty.cpp", "\n")

t.run_build_system()
t.expect_addition("bin/$toolset/debug/hello.exe")
t.rm(".")

# Test a contrived case in which an absolute name is used in a standalone
# project (not Jamfile). Moreover, the target with an absolute name is returned
# via an 'alias' and used from another project.
t.write("a.cpp", "int main() {}\n")
t.write("jamfile.jam", "exe a : /standalone//a ;")
t.write("jamroot.jam", "import standalone ;")
t.write("standalone.jam", """\
import project ;
project.initialize $(__name__) ;
project standalone ;
local pwd = [ PWD ] ;
alias a : $(pwd)/a.cpp ;
""")

t.write("standalone.py", """
from b2.manager import get_manager

# FIXME: this is ugly as death
get_manager().projects().initialize(__name__)

import os ;

# This use of list as parameter is also ugly.
project(['standalone'])

pwd = os.getcwd()
alias('a', [os.path.join(pwd, 'a.cpp')])
""")

t.run_build_system()
t.expect_addition("bin/$toolset/debug/a.exe")

# Test absolute path in target ids.
t.rm(".")

t.write("d1/jamroot.jam", "")
t.write("d1/jamfile.jam", "exe a : a.cpp ;")
t.write("d1/a.cpp", "int main() {}\n")
t.write("d2/jamroot.jam", "")
t.write("d2/jamfile.jam", """\
local pwd = [ PWD ] ;
alias x : $(pwd)/../d1//a ;
""")

t.run_build_system(subdir="d2")
t.expect_addition("d1/bin/$toolset/debug/a.exe")

t.cleanup()

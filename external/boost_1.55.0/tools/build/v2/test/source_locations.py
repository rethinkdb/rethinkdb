#!/usr/bin/python

#  Copyright (C) Craig Rodrigues 2005.
#  Distributed under the Boost Software License, Version 1.0. (See
#  accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)

# Test that projects with multiple source-location directories are handled OK.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("jamroot.jam", """
path-constant SRC1  : "./src1" ;
path-constant SRC2  : "./src2" ;
path-constant SRC3  : "./src3" ;
path-constant BUILD : "build" ;

project : requirements <include>$(SRC1)/include <threading>multi
    : build-dir $(BUILD) ;

build-project project1 ;
""")

t.write("project1/jamfile.jam", """
project project1 : source-location $(SRC1) $(SRC2) $(SRC3) ;
SRCS = s1.cpp s2.cpp testfoo.cpp ;
exe test : $(SRCS) ;
""")

t.write("src1/s1.cpp", "int main() {}\n")
t.write("src2/s2.cpp", "void hello() {}\n")
t.write("src3/testfoo.cpp", "void testfoo() {}\n")

# This file should not be picked up, because "src2" is before "src3" in the list
# of source directories.
t.write("src3/s2.cpp", "void hello() {}\n")

t.run_build_system()

t.cleanup()

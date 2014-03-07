#!/usr/bin/python

# Copyright 2004 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# This test tries to stage the same file to the same location by *two* different
# stage rules, in two different projects. This is not exactly good thing to do,
# but still, V2 should handle this. We had two bugs:
# - since the file is referred from two projects, we created to different
#   virtual targets
# - we also failed to figure out that the two target corresponding to the copied
#   files (created in two projects) are actually equivalent.

import BoostBuild

t = BoostBuild.Tester()

t.write("a.cpp", """
""")

t.write("jamroot.jam", """
build-project a ;
build-project b ;
""")

t.write("a/jamfile.jam", """
stage bin : ../a.cpp : <location>../dist ;
""")

t.write("b/jamfile.jam", """
stage bin : ../a.cpp : <location>../dist ;
""")

t.run_build_system()
t.expect_addition("dist/a.cpp")

t.cleanup()

#!/usr/bin/python

# Copyright 2004 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# This checks that Boost.Build does not reorder <include> properties
# lexicographically.

import BoostBuild

t = BoostBuild.Tester()

t.write("a.cpp", """
#include <a.h>
int main() { foo(); }
""")

t.write("jamroot.jam", """
exe a : a.cpp : <include>d2 <include>d1 ;
""")

t.write("d1/a.h", """
""")

t.write("d2/a.h", """
inline void foo() {}
""")

t.run_build_system()
t.expect_addition("bin/$toolset/debug/a.exe")

t.cleanup()

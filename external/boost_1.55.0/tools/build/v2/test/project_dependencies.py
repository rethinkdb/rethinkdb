#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2002, 2003, 2004 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test that we can specify a dependency property in project requirements, and
# that it will not cause every main target in the project to be generated in its
# own subdirectory.

# The whole test is somewhat moot now.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("jamroot.jam", "build-project src ;")

t.write("lib/jamfile.jam", "lib lib1 : lib1.cpp ;")

t.write("lib/lib1.cpp", """
#ifdef _WIN32
__declspec(dllexport)
#endif
void foo() {}\n
""")

t.write("src/jamfile.jam", """
project : requirements <library>../lib//lib1 ;
exe a : a.cpp ;
exe b : b.cpp ;
""")

t.write("src/a.cpp", """
#ifdef _WIN32
__declspec(dllimport)
#endif
void foo();
int main() { foo(); }
""")

t.copy("src/a.cpp", "src/b.cpp")

t.run_build_system()

# Test that there is no "main-target-a" part.
# t.expect_addition("src/bin/$toolset/debug/a.exe")
# t.expect_addition("src/bin/$toolset/debug/b.exe")

t.cleanup()

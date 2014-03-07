#!/usr/bin/python

# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test that features with default values are always present in build properties
# of any target.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

# Declare *non-propagated* feature foo.
t.write("jamroot.jam", """
import feature : feature ;
feature foo : on off ;
""")

# Note that '<foo>on' will not be propagated to 'd/l'.
t.write("jamfile.jam", """
exe hello : hello.cpp d//l ;
""")

t.write("hello.cpp", """
#ifdef _WIN32
__declspec(dllimport)
#endif
void foo();
int main() { foo(); }
""")

t.write("d/jamfile.jam", """
lib l : l.cpp : <foo>on:<define>FOO ;
""")

t.write("d/l.cpp", """
#ifdef _WIN32
__declspec(dllexport)
#endif
#ifdef FOO
void foo() {}
#endif
""")

t.run_build_system()

t.expect_addition("bin/$toolset/debug/hello.exe")

t.cleanup()

#!/usr/bin/python

# Copyright 2003 Vladimir Prus
# Copyright 2011 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test the C/C++ preprocessor.

import BoostBuild

t = BoostBuild.Tester()

t.write("jamroot.jam", """
project ;
preprocessed hello : hello.cpp ;
preprocessed a : a.c ;
exe hello.exe : hello a : <define>FAIL ;
""")

t.write("hello.cpp", """
#ifndef __cplusplus
#error "This file must be compiled as C++"
#endif
#ifdef FAIL
#error "Not preprocessed?"
#endif
extern "C" int foo();
int main() { return foo(); }
""")

t.write("a.c", """
/* This will not compile unless in C mode. */
#ifdef __cplusplus
#error "This file must be compiled as C"
#endif
#ifdef FAIL
#error "Not preprocessed?"
#endif
int foo()
{
    int new = 0;
    new = (new+1)*7;
    return new;
}
""")

t.run_build_system()
t.expect_addition("bin/$toolset/debug/hello.ii")
t.expect_addition("bin/$toolset/debug/a.i")
t.expect_addition("bin/$toolset/debug/hello.exe")

t.cleanup()

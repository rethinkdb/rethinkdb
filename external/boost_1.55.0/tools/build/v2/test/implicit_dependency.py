#!/usr/bin/python

# Copyright (C) Vladimir Prus 2006.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test the <implicit-dependency> is respected even if the target referred to is
# not built itself, but only referred to by <implicit-dependency>.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("jamroot.jam", """
make a.h : : gen-header ;
explicit a.h ;

exe hello : hello.cpp : <implicit-dependency>a.h ;

import os ;
if [ os.name ] = NT
{
    actions gen-header
    {
       echo int i; > $(<)
    }
}
else
{
    actions gen-header
    {
        echo "int i;" > $(<)
    }
}
""")

t.write("hello.cpp", """
#include "a.h"
int main() { return i; }
""")


t.run_build_system()

t.expect_addition("bin/$toolset/debug/hello.exe")

t.rm("bin")

t.write("jamroot.jam", """
make dir/a.h : : gen-header ;
explicit dir/a.h ;

exe hello : hello.cpp : <implicit-dependency>dir/a.h ;

import os ;
if [ os.name ] = NT
{
    actions gen-header
    {
       echo int i; > $(<)
    }
}
else
{
    actions gen-header
    {
        echo "int i;" > $(<)
    }
}
""")

t.write("hello.cpp", """
#include "dir/a.h"
int main() { return i; }
""")
t.run_build_system()

t.expect_addition("bin/$toolset/debug/hello.exe")

t.cleanup()

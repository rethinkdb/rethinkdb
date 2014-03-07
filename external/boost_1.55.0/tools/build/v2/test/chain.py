#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2002, 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# This tests that :
#  1) the 'make' correctly assigns types to produced targets
#  2) if 'make' creates targets of type CPP, they are correctly used.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

# In order to correctly link this app, 'b.cpp', created by a 'make' rule, should
# be compiled.

t.write("jamroot.jam", "import gcc ;")

t.write("jamfile.jam", r'''
import os ;
if [ os.name ] = NT
{
    actions create
    {
        echo int main() {} > $(<)
    }
}
else
{
    actions create
    {
        echo "int main() {}" > $(<)
    }
}

IMPORT $(__name__) : create : : create ;

exe a : l dummy.cpp ;

# Needs to be a static lib for Windows - main() cannot appear in DLL.
static-lib l : a.cpp b.cpp ;

make b.cpp : : create ;
''')

t.write("a.cpp", "")

t.write("dummy.cpp", "// msvc needs at least one object file\n")

t.run_build_system()

t.expect_addition("bin/$toolset/debug/a.exe")

t.cleanup()

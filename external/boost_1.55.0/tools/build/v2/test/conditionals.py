#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2002, 2003, 2004 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test conditional properties.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

# Arrange a project which will build only if 'a.cpp' is compiled with "STATIC"
# define.
t.write("a.cpp", """\
#ifdef STATIC
int main() {}
#endif
""")

# Test conditionals in target requirements.
t.write("jamroot.jam", "exe a : a.cpp : <link>static:<define>STATIC ;")
t.run_build_system(["link=static"])
t.expect_addition("bin/$toolset/debug/link-static/a.exe")
t.rm("bin")

# Test conditionals in project requirements.
t.write("jamroot.jam", """
project : requirements <link>static:<define>STATIC ;
exe a : a.cpp ;
""")
t.run_build_system(["link=static"])
t.expect_addition("bin/$toolset/debug/link-static/a.exe")
t.rm("bin")

# Regression test for a bug found by Ali Azarbayejani. Conditionals inside
# usage requirement were not being evaluated.
t.write("jamroot.jam", """
lib l : l.cpp : : : <link>static:<define>STATIC ;
exe a : a.cpp l ;
""")
t.write("l.cpp", "int i;")
t.run_build_system(["link=static"])
t.expect_addition("bin/$toolset/debug/link-static/a.exe")

t.cleanup()

#!/usr/bin/python
#
# Copyright (c) 2008 Steven Watanabe
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt) or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

tester = BoostBuild.Tester(use_test_config=False)

tester.write("jamroot.jam", """
obj test : test.cpp : <include>a&&b ;
""")

tester.write("test.cpp", """
#include <test1.hpp>
#include <test2.hpp>
int main() {}
""")

tester.write("a/test1.hpp", """
""")

tester.write("b/test2.hpp", """
""")

tester.run_build_system()

tester.expect_addition("bin/$toolset/debug/test.obj")

tester.touch("a/test1.hpp")
tester.run_build_system()
tester.expect_touch("bin/$toolset/debug/test.obj")

tester.touch("b/test2.hpp")
tester.run_build_system()
tester.expect_touch("bin/$toolset/debug/test.obj")

tester.cleanup()

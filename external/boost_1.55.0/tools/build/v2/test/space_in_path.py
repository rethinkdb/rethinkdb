#!/usr/bin/python

# Copyright 2012 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test that paths containing spaces are handled correctly by actions.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("has space/jamroot.jam", """\
import testing ;
unit-test test : test.cpp ;
""")
t.write("has space/test.cpp", "int main() {}\n")

t.run_build_system(["has space"])

t.cleanup()

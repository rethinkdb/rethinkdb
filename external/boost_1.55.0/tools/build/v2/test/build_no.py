#!/usr/bin/python

# Copyright (C) Vladimir Prus 2006.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Tests that <build>no property prevents a target from being built.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("jamroot.jam", "exe hello : hello.cpp : <variant>debug:<build>no ;")
t.write("hello.cpp", "int main() {}\n")

t.run_build_system()
t.expect_nothing_more()

t.run_build_system(["release"])
t.expect_addition("bin/$toolset/release/hello.exe")

t.cleanup()

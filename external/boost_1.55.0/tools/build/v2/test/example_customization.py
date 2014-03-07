#!/usr/bin/python

# Copyright (C) Vladimir Prus 2006.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test the 'customization' example.

import BoostBuild

t = BoostBuild.Tester()

t.set_tree("../example/customization")

t.run_build_system()

t.expect_addition(["bin/$toolset/debug/codegen.exe",
                   "bin/$toolset/debug/usage.cpp"])

t.cleanup()

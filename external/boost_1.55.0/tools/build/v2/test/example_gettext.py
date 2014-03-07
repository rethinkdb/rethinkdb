#!/usr/bin/python

# Copyright (C) Vladimir Prus 2006.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test the 'gettext' example.

import BoostBuild
import os
import string

t = BoostBuild.Tester()

t.set_tree("../example/gettext")

t.run_build_system(stderr=None)

t.expect_addition(["bin/$toolset/debug/main.exe",
                   "bin/$toolset/debug/russian.mo"])

file = t.adjust_names(["bin/$toolset/debug/main.exe"])[0]

input_fd = os.popen(file)
input = input_fd.read();

t.fail_test(string.find(input, "international hello") != 0)

t.cleanup()

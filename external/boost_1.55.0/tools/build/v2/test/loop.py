#!/usr/bin/python

# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild
import string

t = BoostBuild.Tester()

t.write("jamroot.jam", """\
lib main : main.cpp l ;
lib l : l.cpp main ;
""")

t.write("main.cpp", "")
t.write("l.cpp", "")

t.run_build_system(["--no-error-backtrace"], status=1)
t.fail_test(string.find(t.stdout(),
    "error: Recursion in main target references") == -1)

t.cleanup()

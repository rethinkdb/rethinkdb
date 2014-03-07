#!/usr/bin/python

# Copyright 2002, 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# This tests correct handling of "-d1" and "-d2" options.

import BoostBuild

t = BoostBuild.Tester(["-ffile.jam"], pass_d0=False, pass_toolset=0)

t.write("file.jam", """\
actions a { }
actions quietly b { }
ALWAYS all ;
a all ;
b all ;
""")

t.run_build_system(["-d0"], stdout="")

t.run_build_system(["-d1"])
t.expect_output_lines("a all")
t.expect_output_lines("b all", False)

t.run_build_system(["-d2"])
t.expect_output_lines("a all")
t.expect_output_lines("b all")

t.cleanup()

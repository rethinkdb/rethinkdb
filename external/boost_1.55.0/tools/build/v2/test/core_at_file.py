#!/usr/bin/python

# Copyright 2011 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)


import BoostBuild

t = BoostBuild.Tester(["-ffile.jam"], pass_toolset=0)

t.write("file.jam", """\
name = n1 n2 ;
contents = M1 M2 ;
EXIT file: "@(o$(name) .txt:E= test -D$(contents))" : 0 ;
""")

t.run_build_system()
t.expect_output_lines("file: on1 on2 .txt");
t.expect_addition("on1 on2 .txt")
t.expect_content("on1 on2 .txt", " test -DM1 -DM2", True)

t.rm(".")

t.write("file.jam", """\
name = n1 n2 ;
contents = M1 M2 ;
actions run { echo file: "@(o$(name) .txt:E= test -D$(contents))" }
run all ;
""")

t.run_build_system(["-d2"])
t.expect_output_lines(' echo file: "on1 on2 .txt" ');
t.expect_addition("on1 on2 .txt")
t.expect_content("on1 on2 .txt", " test -DM1 -DM2", True)

t.rm(".")

t.write("file.jam", """\
name = n1 n2 ;
contents = M1 M2 ;
file = "@($(STDOUT):E= test -D$(contents)\n)" ;
actions run { $(file) }
run all ;
""")

t.run_build_system(["-d1"])
t.expect_output_lines(" test -DM1 -DM2")

t.rm(".")

t.write("file.jam", """\
name = n1 n2 ;
contents = M1 M2 ;
actions run { @($(STDOUT):E= test -D$(contents)\n) }
run all ;
""")

t.run_build_system(["-d1"])
t.expect_output_lines(" test -DM1 -DM2")

t.cleanup()

#!/usr/bin/python

# Copyright 2003, 2006 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("jamroot.jam", """\
project : requirements <link>static ;
exe a : a.cpp [ lib helper : helper.cpp ] ;
""")

t.write("a.cpp", """\
extern void helper();
int main() {}
""")

t.write("helper.cpp", "void helper() {}\n")

t.run_build_system()
t.expect_addition("bin/$toolset/debug/link-static/a__helper.lib")
t.rm("bin/$toolset/debug/link-static/a__helper.lib")

t.run_build_system(["a__helper"])
t.expect_addition("bin/$toolset/debug/link-static/a__helper.lib")

t.rm("bin")


# Now check that inline targets with the same name but present in different
# places are not confused between each other, and with top-level targets.
t.write("jamroot.jam", """\
project : requirements <link>static ;
exe a : a.cpp [ lib helper : helper.cpp ] ;
exe a2 : a.cpp [ lib helper : helper.cpp ] ;
""")

t.run_build_system()
t.expect_addition("bin/$toolset/debug/link-static/a.exe")
t.expect_addition("bin/$toolset/debug/link-static/a__helper.lib")
t.expect_addition("bin/$toolset/debug/link-static/a2__helper.lib")


# Check that the 'alias' target does not change the name of inline targets, and
# that inline targets are explicit.
t.write("jamroot.jam", """\
project : requirements <link>static ;
alias a : [ lib helper : helper.cpp ] ;
explicit a ;
""")
t.rm("bin")

t.run_build_system()
t.expect_nothing_more()

t.run_build_system(["a"])
t.expect_addition("bin/$toolset/debug/link-static/helper.lib")

t.cleanup()

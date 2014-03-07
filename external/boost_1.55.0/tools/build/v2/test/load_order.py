#!/usr/bin/python

# Copyright 2004 Vladimir Prus.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test that we load parent projects before loading children.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("jamroot.jam", """\
use-project /child : child ;
ECHO "Setting parent requirements" ;
project : requirements <define>PASS_THE_TEST ;
alias x : child//main ;
""")

t.write("child/jamfile.jam", """\
ECHO "Setting child requirements" ;
project /child ;
exe main : main.cpp ;
""")

t.write("child/main.cpp", """\
#if defined(PASS_THE_TEST)
int main() {}
#endif
""")

t.run_build_system()

t.expect_addition("child/bin/$toolset/debug/main.exe")
t.fail_test(t.stdout().find("Setting child requirements") < t.stdout().find(
    "Setting parent requirements"))


# Regression test: parent requirements were ignored in some cases.
t.rm(".")
t.write("jamroot.jam", "build-project src ;")
t.write("src/jamfile.jam", "project : requirements <define>EVERYTHING_OK ;")
t.write("src/app/jamfile.jam", "exe test : test.cpp ;")
t.write("src/app/test.cpp", """\
#ifdef EVERYTHING_OK
int main() {}
#endif
""")

t.run_build_system(subdir="src/app")
t.expect_addition("src/app/bin/$toolset/debug/test.exe")


# child/child2 used to be loaded before child
t.rm(".")
t.write("jamroot.jam", """\
use-project /child/child2 : child/child2 ;
rule parent-rule ( )
{
  ECHO "Running parent-rule" ;
}
""")
t.write("child/jamfile.jam", "")
t.write("child/child1/jamfile.jam", "")
t.write("child/child2/jamfile.jam", "parent-rule ;")

t.run_build_system(subdir="child/child1")
t.expect_output_lines("Running parent-rule")

t.cleanup()

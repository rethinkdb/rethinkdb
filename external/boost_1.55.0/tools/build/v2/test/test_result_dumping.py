#!/usr/bin/python

# Copyright 2008 Jurko Gospodnetic
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Tests dumping Boost Build based testing results.

import BoostBuild


###############################################################################
#
#   Test that dumping Boost Build based testing results works in case test code
# is not located in a folder under the Jamroot folder.
#
###############################################################################

t = BoostBuild.Tester(["--dump-tests"], use_test_config=False)

t.write("TestBuild/jamroot.jam", """\
import testing ;
test-suite testit : [ run ../TestSource/test.cpp ] ;
""")

t.write("TestSource/test.cpp", "int main() {}\n")

t.run_build_system(subdir="TestBuild")
t.expect_output_lines('boost-test(RUN) "*/TestBuild/test" : '
    '"../TestSource/test.cpp"')

t.cleanup()

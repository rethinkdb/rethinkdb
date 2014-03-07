#!/usr/bin/python

# Copyright 2008 Jurko Gospodnetic
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Tests different aspects of Boost Builds automated testing support.

import BoostBuild


################################################################################
#
# test_files_with_spaces_in_their_name()
# --------------------------------------
#
################################################################################

def test_files_with_spaces_in_their_name():
    """Regression test making sure test result files get created correctly when
    testing files with spaces in their name.
    """

    t = BoostBuild.Tester(use_test_config=False)

    t.write("valid source.cpp", "int main() {}\n");

    t.write("invalid source.cpp", "this is not valid source code");
    
    t.write("jamroot.jam", """
import testing ;
testing.compile "valid source.cpp" ;
testing.compile-fail "invalid source.cpp" ;
""")

    t.run_build_system(status=0)
    t.expect_addition("bin/invalid source.test/$toolset/debug/invalid source.obj")
    t.expect_addition("bin/invalid source.test/$toolset/debug/invalid source.test")
    t.expect_addition("bin/valid source.test/$toolset/debug/valid source.obj")
    t.expect_addition("bin/valid source.test/$toolset/debug/valid source.test")

    t.expect_content("bin/valid source.test/$toolset/debug/valid source.test", \
        "passed" )
    t.expect_content( \
        "bin/invalid source.test/$toolset/debug/invalid source.test", \
        "passed" )
    t.expect_content( \
        "bin/invalid source.test/$toolset/debug/invalid source.obj", \
        "failed as expected" )

    t.cleanup()


################################################################################
#
# main()
# ------
#
################################################################################

test_files_with_spaces_in_their_name()

#!/usr/bin/python

# Copyright (C) Vladimir Prus 2005.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Tests that we can declare a rule in Jamroot that will be can be called in
# child Jamfile to declare a target. Specifically test for use of 'glob' in that
# rule.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)


t.write("jamroot.jam", """
project : requirements <link>static ;
rule my-lib ( name ) { lib $(name) : [ glob *.cpp ] ; }
""")

t.write("sub/a.cpp", """
""")

t.write("sub/jamfile.jam", """
my-lib foo ;
""")


t.run_build_system(subdir="sub")

t.expect_addition("sub/bin/$toolset/debug/link-static/foo.lib")

t.cleanup()

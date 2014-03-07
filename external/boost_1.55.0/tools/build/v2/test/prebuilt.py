#!/usr/bin/python

# Copyright 2002, 2003, 2004 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test that we can use already built sources

import BoostBuild

t = BoostBuild.Tester(["debug", "release"], use_test_config=False)

t.set_tree('prebuilt')

t.expand_toolset("ext/jamroot.jam")
t.expand_toolset("jamroot.jam")

# First, build the external project.
t.run_build_system(subdir="ext")

# Then pretend that we do not have the sources for the external project, and
# can only use compiled binaries.
t.copy("ext/jamfile2.jam", "ext/jamfile.jam")
t.expand_toolset("ext/jamfile.jam")

# Now check that we can build the main project, and that correct prebuilt file
# is picked, depending of variant. This also checks that correct includes for
# prebuilt libraries are used.
t.run_build_system()
t.expect_addition("bin/$toolset/debug/hello.exe")
t.expect_addition("bin/$toolset/release/hello.exe")

t.rm("bin")


# Now test that prebuilt file specified by absolute name works too.
t.copy("ext/jamfile3.jam", "ext/jamfile.jam")
t.expand_toolset("ext/jamfile.jam")
t.run_build_system()
t.expect_addition("bin/$toolset/debug/hello.exe")
t.expect_addition("bin/$toolset/release/hello.exe")

t.cleanup()

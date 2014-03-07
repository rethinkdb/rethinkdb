#!/usr/bin/python

# Copyright (C) Vladimir Prus 2005.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Tests that we can build a project when the current directory is outside of
# that project tree, that is 'bjam some_dir' works.

import BoostBuild

# Create a temporary working directory.
t = BoostBuild.Tester(use_test_config=False)

# Create the needed files.
t.write("p1/jamroot.jam", "exe hello : hello.cpp ;")
t.write("p1/hello.cpp", "int main() {}\n")
t.write("p2/jamroot.jam", """\
exe hello2 : hello.cpp ;
exe hello3 : hello.cpp ;
""")
t.write("p2/hello.cpp", "int main() {}\n")

t.run_build_system(["p1", "p2//hello3"])
t.expect_addition("p1/bin/$toolset/debug/hello.exe")
t.expect_addition("p2/bin/$toolset/debug/hello3.exe")

t.cleanup()

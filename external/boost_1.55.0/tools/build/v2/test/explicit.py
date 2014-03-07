#!/usr/bin/python

# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("jamroot.jam", """\
exe hello : hello.cpp ;
exe hello2 : hello.cpp ;
explicit hello2 ;
""")

t.write("hello.cpp", "int main() {}\n")

t.run_build_system()
t.ignore("*.tds")
t.expect_addition(BoostBuild.List("bin/$toolset/debug/hello") * \
    [".exe", ".obj"])
t.expect_nothing_more()

t.run_build_system(["hello2"])
t.expect_addition("bin/$toolset/debug/hello2.exe")

t.rm(".")


# Test that 'explicit' used in a helper rule applies to the current project, and
# not to the Jamfile where the helper rule is defined.
t.write("jamroot.jam", """\
rule myinstall ( name : target )
{
    install $(name)-bin : $(target) ;
    explicit $(name)-bin ;
    alias $(name) : $(name)-bin ;
}
""")

t.write("sub/a.cpp", "\n")
t.write("sub/jamfile.jam", "myinstall dist : a.cpp ;")

t.run_build_system(subdir="sub")
t.expect_addition("sub/dist-bin/a.cpp")

t.rm("sub/dist-bin")

t.write("sub/jamfile.jam", """\
myinstall dist : a.cpp ;
explicit dist ;
""")

t.run_build_system(subdir="sub")
t.expect_nothing_more()

t.cleanup()

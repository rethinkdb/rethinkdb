#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2002, 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test that default build clause actually has any effect.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("jamroot.jam", "")
t.write("jamfile.jam", "exe a : a.cpp : : debug release ;")
t.write("a.cpp", "int main() {}\n")

t.run_build_system()
t.expect_addition("bin/$toolset/debug/a.exe")
t.expect_addition("bin/$toolset/release/a.exe")

# Check that explictly-specified build variant supresses default-build.
t.rm("bin")
t.run_build_system(["release"])
t.expect_addition(BoostBuild.List("bin/$toolset/release/") * "a.exe a.obj")
t.expect_nothing_more()

# Now check that we can specify explicit build request and default-build will be
# combined with it.
t.run_build_system(["optimization=space"])
t.expect_addition("bin/$toolset/debug/optimization-space/a.exe")
t.expect_addition("bin/$toolset/release/optimization-space/a.exe")

# Test that default-build must be identical in all alternatives. Error case.
t.write("jamfile.jam", """\
exe a : a.cpp : : debug ;
exe a : b.cpp : : ;
""")
t.run_build_system(["-n", "--no-error-backtrace"], status=1)
t.fail_test(t.stdout().find("default build must be identical in all alternatives") == -1)

# Test that default-build must be identical in all alternatives. No Error case,
# empty default build.
t.write("jamfile.jam", """\
exe a : a.cpp : <variant>debug ;
exe a : b.cpp : <variant>release ;
""")
t.run_build_system(["-n", "--no-error-backtrace"], status=0)

# Now try a harder example: default build which contains <define> should cause
# <define> to be present when "b" is compiled. This happens only if
# "build-project b" is placed first.
t.write("jamfile.jam", """\
project : default-build <define>FOO ;
build-project a ;
build-project b ;
""")

t.write("a/jamfile.jam", "exe a : a.cpp ../b//b ;")
t.write("a/a.cpp", """\
#ifdef _WIN32
__declspec(dllimport)
#endif
void foo();
int main() { foo(); }
""")

t.write("b/jamfile.jam", "lib b : b.cpp ;")
t.write("b/b.cpp", """\
#ifdef FOO
#ifdef _WIN32
__declspec(dllexport)
#endif
void foo() {}
#endif
""")

t.run_build_system()

t.cleanup()

#!/usr/bin/python
#
# Copyright (c) 2008 Steven Watanabe
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt) or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

tester = BoostBuild.Tester(use_test_config=False)


################################################################################
#
# Test without giving the project an explicit id.
#
################################################################################

tester.write("jamroot.jam", """
lib test : test.cpp ;
project : requirements <library>test ;
build-project a ;
""")

tester.write("test.cpp", """
#ifdef _WIN32
    __declspec(dllexport)
#endif
void foo() {}
""")

tester.write("a/test1.cpp", """
int main() {}
""")

tester.write("a/jamfile.jam", """
exe test1 : test1.cpp ;
""")

tester.run_build_system()

tester.expect_addition("bin/$toolset/debug/test.obj")
tester.expect_addition("a/bin/$toolset/debug/test1.exe")

tester.rm("bin")
tester.rm("a/bin")


################################################################################
#
# Run the same test from the "a" directory.
#
################################################################################

tester.run_build_system(subdir="a")

tester.expect_addition("bin/$toolset/debug/test.obj")
tester.expect_addition("a/bin/$toolset/debug/test1.exe")

tester.rm("bin")
tester.rm("a/bin")


################################################################################
#
# This time, do give the project an id.
#
################################################################################

tester.write("jamroot.jam", """
lib test : test.cpp ;
project test_project : requirements <library>test ;
build-project a ;
""")

tester.run_build_system()

tester.expect_addition("bin/$toolset/debug/test.obj")
tester.expect_addition("a/bin/$toolset/debug/test1.exe")

tester.rm("bin")
tester.rm("a/bin")


################################################################################
#
# Now, give the project an id in its attributes.
#
################################################################################

tester.write("jamroot.jam", """
lib test : test.cpp ;
project : id test_project : requirements <library>test ;
build-project a ;
""")

tester.run_build_system()

tester.expect_addition("bin/$toolset/debug/test.obj")
tester.expect_addition("a/bin/$toolset/debug/test1.exe")

tester.rm("bin")
tester.rm("a/bin")


################################################################################
#
# Give the project an id in both ways at once.
#
################################################################################

tester.write("jamroot.jam", """
lib test : test.cpp ;
project test_project1 : id test_project : requirements <library>test ;
build-project a ;
""")

tester.run_build_system()

tester.expect_addition("bin/$toolset/debug/test.obj")
tester.expect_addition("a/bin/$toolset/debug/test1.exe")

tester.rm("bin")
tester.rm("a/bin")


################################################################################
#
# Test an absolute path in native format.
#
################################################################################

tester.write("jamroot.jam", """
import path ;
path-constant here : . ;
current-location = [ path.native [ path.root [ path.make $(here) ] [ path.pwd ]
    ] ] ;
project test : requirements <source>$(current-location)/a/test1.cpp ;
exe test : test.cpp ;
""")

tester.run_build_system()
tester.expect_addition("bin/$toolset/debug/test.exe")

tester.rm("bin")
tester.rm("a/bin")


################################################################################
#
# Test an absolute path in canonical format.
#
################################################################################

tester.write("jamroot.jam", """
import path ;
path-constant here : . ;
current-location = [ path.root [ path.make $(here) ] [ path.pwd ] ] ;
project test : requirements <source>$(current-location)/a/test1.cpp ;
exe test : test.cpp ;
""")

tester.run_build_system()
tester.expect_addition("bin/$toolset/debug/test.exe")

tester.rm("bin")
tester.rm("a/bin")


################################################################################
#
# Test dependency properties (e.g. <source>) whose targets are specified using a
# relative path.
#
################################################################################

# Use jamroot.jam rather than jamfile.jam to avoid inheriting the <source> from
# the parent as that would would make test3 a source of itself.
tester.write("b/jamroot.jam", """
obj test3 : test3.cpp ;
""")

tester.write("b/test3.cpp", """
void bar() {}
""")

tester.write("jamroot.jam", """
project test : requirements <source>b//test3 ;
build-project a ;
""")

tester.write("a/jamfile.jam", """
exe test : test1.cpp ;
""")

tester.write("a/test1.cpp", """
void bar();
int main() { bar(); }
""")

tester.run_build_system()
tester.expect_addition("b/bin/$toolset/debug/test3.obj")
tester.expect_addition("a/bin/$toolset/debug/test.exe")

tester.rm("bin")
tester.rm("a")
tester.rm("jamroot.jam")
tester.rm("test.cpp")


################################################################################
#
# Test that source-location is respected.
#
################################################################################

tester.write("build/jamroot.jam", """
project : requirements <source>test.cpp : source-location ../src ;
""")

tester.write("src/test.cpp", """
int main() {}
""")

tester.write("build/a/jamfile.jam", """
project : source-location ../../a_src ;
exe test : test1.cpp ;
""")

tester.write("a_src/test1.cpp", """
""")

tester.run_build_system(subdir="build/a")
tester.expect_addition("build/a/bin/$toolset/debug/test.exe")

tester.cleanup()

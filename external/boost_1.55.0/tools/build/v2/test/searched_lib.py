#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2003, 2004, 2005, 2006 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test usage of searched-libs: one which are found via -l
# switch to the linker/compiler.

import BoostBuild
import os
import string

t = BoostBuild.Tester(use_test_config=False)


# To start with, we have to prepare a library to link with.
t.write("lib/jamroot.jam", "")
t.write("lib/jamfile.jam", "lib test_lib : test_lib.cpp ;")
t.write("lib/test_lib.cpp", """\
#ifdef _WIN32
__declspec(dllexport)
#endif
void foo() {}
""");

t.run_build_system(subdir="lib")
t.expect_addition("lib/bin/$toolset/debug/test_lib.dll")


# Auto adjusting of suffixes does not work, since we need to
# change dll to lib.
if ( ( os.name == "nt" ) or os.uname()[0].lower().startswith("cygwin") ) and \
    ( BoostBuild.get_toolset() != "gcc" ):
    t.copy("lib/bin/$toolset/debug/test_lib.implib", "lib/test_lib.implib")
    t.copy("lib/bin/$toolset/debug/test_lib.dll", "lib/test_lib.dll")
else:
    t.copy("lib/bin/$toolset/debug/test_lib.dll", "lib/test_lib.dll")


# Test that the simplest usage of searched library works.
t.write("jamroot.jam", "")

t.write("jamfile.jam", """\
import path ;
import project ;
exe main : main.cpp helper ;
lib helper : helper.cpp test_lib ;
lib test_lib : : <name>test_lib <search>lib ;
""")

t.write("main.cpp", """\
void helper();
int main() { helper(); }
""")

t.write("helper.cpp", """\
void foo();
void
#if defined(_WIN32)
__declspec(dllexport)
#endif
helper() { foo(); }
""")

t.run_build_system(["-d2"])
t.expect_addition("bin/$toolset/debug/main.exe")
t.rm("bin/$toolset/debug/main.exe")


# Test that 'unit-test' will correctly add runtime paths to searched libraries.
t.write("jamfile.jam", """\
import path ;
import project ;
import testing ;

project : requirements <hardcode-dll-paths>false ;

unit-test main : main.cpp helper ;
lib helper : helper.cpp test_lib ;
lib test_lib : : <name>test_lib <search>lib ;
""")

t.run_build_system()
t.expect_addition("bin/$toolset/debug/main.passed")
t.rm("bin/$toolset/debug/main.exe")


# Now try using searched lib from static lib. Request shared version of searched
# lib, since we do not have a static one handy.
t.write("jamfile.jam", """\
exe main : main.cpp helper ;
lib helper : helper.cpp test_lib/<link>shared : <link>static ;
lib test_lib : : <name>test_lib <search>lib ;
""")

t.run_build_system(stderr=None)
t.expect_addition("bin/$toolset/debug/main.exe")
t.expect_addition("bin/$toolset/debug/link-static/helper.lib")
t.rm("bin/$toolset/debug/main.exe")

# A regression test: <library>property referring to searched-lib was being
# mishandled. As the result, we were putting target name to the command line!
# Note that
#    g++ ...... <.>z
# works nicely in some cases, sending output from compiler to file 'z'. This
# problem shows up when searched libs are in usage requirements.
t.write("jamfile.jam", "exe main : main.cpp d/d2//a ;")
t.write("main.cpp", """\
void foo();
int main() { foo(); }
""")

t.write("d/d2/jamfile.jam", """\
lib test_lib : : <name>test_lib <search>../../lib ;
lib a : a.cpp : : : <library>test_lib ;
""")

t.write("d/d2/a.cpp", """\
#ifdef _WIN32
__declspec(dllexport) int force_library_creation_for_a;
#endif
""")

t.run_build_system()


# A regression test. Searched targets were not associated with any properties.
# For that reason, if the same searched lib is generated with two different
# properties, we had an error saying they are actualized to the same Jam target
# name.
t.write("jamroot.jam", "")

t.write("a.cpp", "")

# The 'l' library will be built in two variants: 'debug' (directly requested)
# and 'release' (requested from 'a').
t.write("jamfile.jam", """\
exe a : a.cpp l/<variant>release ;
lib l : : <name>l_d <variant>debug ;
lib l : : <name>l_r <variant>release ;
""")

t.run_build_system(["-n"])


# A regression test. Two virtual target with the same properties were created
# for 'l' target, which caused and error to be reported when actualizing
# targets. The final error is correct, but we should not create two duplicated
# targets. Thanks to Andre Hentz for finding this bug.
t.write("jamroot.jam", "")
t.write("a.cpp", "")
t.write("jamfile.jam", """\
project a : requirements <runtime-link>static ;
static-lib a : a.cpp l ;
lib l : : <name>l_f ;
""")

t.run_build_system(["-n"])


# Make sure plain "lib foobar ; " works.
t.write("jamfile.jam", """\
exe a : a.cpp foobar ;
lib foobar ;
""")

t.run_build_system(["-n", "-d2"])
t.fail_test(string.find(t.stdout(), "foobar") == -1)


# Make sure plain "lib foo bar ; " works.
t.write("jamfile.jam", """\
exe a : a.cpp foo bar ;
lib foo bar ;
""")

t.run_build_system(["-n", "-d2"])
t.fail_test(string.find(t.stdout(), "foo") == -1)
t.fail_test(string.find(t.stdout(), "bar") == -1)

t.cleanup()

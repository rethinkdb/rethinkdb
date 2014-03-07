#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2002, 2003, 2004 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

def test_basic():
    t = BoostBuild.Tester(use_test_config=False)

    t.write("jamroot.jam", "lib a : a.cpp : <include>. ;")
    t.write("a.cpp", """\
#include <a.h>
void
# ifdef _WIN32
__declspec(dllexport)
# endif
foo() {}
""")
    t.write("a.h", "//empty file\n")
    t.write("d/jamfile.jam", "exe b : b.cpp ..//a ;")
    t.write("d/b.cpp", """\
void foo();
int main() { foo(); }
""")
    t.run_build_system(subdir="d")

    # Path features with condition.
    t.write("jamroot.jam", "lib a : a.cpp : <variant>debug:<include>. ;")
    t.rm("bin")
    t.run_build_system(subdir="d")


    # Path features with condition in usage requirements.
    t.write("jamroot.jam", """\
lib a : a.cpp : <include>. : : <variant>debug:<include>. ;
""")
    t.write("d/b.cpp", """\
#include <a.h>
void foo();
int main() { foo(); }
""")
    t.rm("d/bin")
    t.run_build_system(subdir="d")

    t.cleanup()


def test_absolute_paths():
    """
      Test that absolute paths inside requirements are ok. The problems
    appeared only when building targets in subprojects.

    """
    t = BoostBuild.Tester(use_test_config=False)

    t.write("jamroot.jam", "build-project x ;")
    t.write("x/jamfile.jam", """\
local pwd = [ PWD ] ;
project : requirements <include>$(pwd)/x/include ;
exe m : m.cpp : <include>$(pwd)/x/include2 ;
""")
    t.write("x/m.cpp", """\
#include <h1.hpp>
#include <h2.hpp>
int main() {}
""")
    t.write("x/include/h1.hpp", "\n")
    t.write("x/include2/h2.hpp", "\n")

    t.run_build_system()
    t.expect_addition("x/bin/$toolset/debug/m.exe")

    t.cleanup()


def test_ordered_paths():
    """Test that "&&" in path features is handled correctly."""

    t = BoostBuild.Tester(use_test_config=False)

    t.write("jamroot.jam", "build-project sub ;")
    t.write("sub/jamfile.jam", "exe a : a.cpp : <include>../h1&&../h2 ;")
    t.write("sub/a.cpp", """\
#include <header.h>
int main() { return OK; }
""")
    t.write("h2/header.h", "int const OK = 0;\n")
    t.run_build_system()
    t.expect_addition("sub/bin/$toolset/debug/a.exe")

    t.cleanup()


def test_paths_set_by_indirect_conditionals():
    t = BoostBuild.Tester(pass_d0=False, use_test_config=False)

    header = "child_dir/folder_to_include/some_header.h"

    t.write("jamroot.jam", "build-project child_dir ;")
    t.write("child_dir/jamfile.jam", """\
import remote/remote ;

# If we set the <include>folder_to_include property directly, it will work
obj x1 : x.cpp : <conditional>@attach-include-local ;
obj x2 : x.cpp : <conditional>@remote/remote.attach-include-remote ;

rule attach-include-local ( properties * )
{
    return <include>folder_to_include ;
}
""")
    t.write("child_dir/remote/remote.jam", """\
rule attach-include-remote ( properties * )
{
    return <include>folder_to_include ;
}
""")
    t.write("child_dir/x.cpp", """\
#include <some_header.h>
int main() {}
""")
    t.write(header, "int some_func();\n")
    t.write("child_dir/folder_to_include/jamfile.jam", "")

    expected_x1 = "child_dir/bin/$toolset/debug/x1.obj"
    expected_x2 = "child_dir/bin/$toolset/debug/x2.obj"

    t.run_build_system()
    t.expect_addition(expected_x1)
    t.expect_addition(expected_x2)

    t.touch(header)
    t.run_build_system(subdir="child_dir")
    t.expect_touch(expected_x1)
    t.expect_touch(expected_x2)

    t.touch(header)
    t.run_build_system(["..", "-d2"], subdir="child_dir/folder_to_include")
    t.expect_touch(expected_x1)
    t.expect_touch(expected_x2)

    t.cleanup()


test_basic()
test_absolute_paths()
test_ordered_paths()
test_paths_set_by_indirect_conditionals()
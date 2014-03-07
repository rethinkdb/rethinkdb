#!/usr/bin/python

# Copyright (C) Vladimir Prus 2003.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#         http://www.boost.org/LICENSE_1_0.txt)

# Test for the regression testing framework.

import BoostBuild

# Create a temporary working directory.
t = BoostBuild.Tester(use_test_config=False)

t.write("c.cpp", "\n")
t.write("r.cpp", """\
void helper();

#include <iostream>
int main( int ac, char * av[] )
{
    helper();
    for ( int i = 1; i < ac; ++i )
       std::cout << av[ i ] << '\\n';
}
""")
t.write("c-f.cpp", "int\n")
t.write("r-f.cpp", "int main() { return 1; }\n")

t.write("jamroot.jam", "")
t.write("jamfile.jam", """\
import testing ;
compile c.cpp ;
compile-fail c-f.cpp ;
run r.cpp libs//helper : foo bar ;
run-fail r-f.cpp ;
""")

t.write("libs/jamfile.jam", "lib helper : helper.cpp ;")
t.write("libs/helper.cpp", """\
void
#if defined(_WIN32)
__declspec(dllexport)
#endif
helper() {}
""")

# First test that when outcomes are expected, all .test files are created.
t.run_build_system(["hardcode-dll-paths=false"], stderr=None, status=None)
t.expect_addition("bin/c.test/$toolset/debug/c.test")
t.expect_addition("bin/c-f.test/$toolset/debug/c-f.test")
t.expect_addition("bin/r.test/$toolset/debug/r.test")
t.expect_addition("bin/r-f.test/$toolset/debug/r-f.test")

# Make sure args are handled.
t.expect_content("bin/r.test/$toolset/debug/r.output",
                 "foo\nbar\n*\nEXIT STATUS: 0*\n", True)

# Test that input file is handled as well.
t.write("r.cpp", """\
#include <iostream>
#include <fstream>
int main( int ac, char * av[] )
{
    for ( int i = 1; i < ac; ++i )
    {
        std::ifstream ifs( av[ i ] );
        std::cout << ifs.rdbuf();
    }
}
""")

t.write("dir/input.txt", "test input")

t.write("jamfile.jam", """\
import testing ;
compile c.cpp ;
obj c-obj : c.cpp ;
compile-fail c-f.cpp ;
run r.cpp : : dir/input.txt ;
run-fail r-f.cpp ;
time execution : r ;
time compilation : c-obj ;
""")

t.run_build_system(["hardcode-dll-paths=false"])
t.expect_content("bin/r.test/$toolset/debug/r.output", """\
test input
EXIT STATUS: 0
""")

t.expect_addition('bin/$toolset/debug/execution.time')
t.expect_addition('bin/$toolset/debug/compilation.time')

# Make sure test failures are detected. Reverse expectation and see if .test
# files are created or not.
t.write("jamfile.jam", """\
import testing ;
compile-fail c.cpp ;
compile c-f.cpp ;
run-fail r.cpp : : dir/input.txt ;
run r-f.cpp ;
""")

t.touch(BoostBuild.List("c.cpp c-f.cpp r.cpp r-f.cpp"))

t.run_build_system(["hardcode-dll-paths=false"], stderr=None, status=1)
t.expect_removal("bin/c.test/$toolset/debug/c.test")
t.expect_removal("bin/c-f.test/$toolset/debug/c-f.test")
t.expect_removal("bin/r.test/$toolset/debug/r.test")
t.expect_removal("bin/r-f.test/$toolset/debug/r-f.test")

t.cleanup()

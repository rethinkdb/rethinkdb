#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2003, 2006 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test main target alternatives.

import BoostBuild
import string

t = BoostBuild.Tester(use_test_config=False)

# Test that basic alternatives selection works.
t.write("jamroot.jam", "")

t.write("jamfile.jam", """
exe a : a_empty.cpp ;
exe a : a.cpp : <variant>release ;
""")

t.write("a_empty.cpp", "")

t.write("a.cpp", "int main() {}\n")

t.run_build_system(["release"])

t.expect_addition("bin/$toolset/release/a.exe")

# Test that alternative selection works for ordinary properties, in particular
# user-defined.
t.write("jamroot.jam", "")

t.write("jamfile.jam", """
import feature ;
feature.feature X : off on : propagated ;
exe a : b.cpp ;
exe a : a.cpp : <X>on ;
""")
t.write("b.cpp", "int main() {}\n")

t.rm("bin")

t.run_build_system()
t.expect_addition("bin/$toolset/debug/b.obj")

t.run_build_system(["X=on"])
t.expect_addition("bin/$toolset/debug/X-on/a.obj")

t.rm("bin")

# Test that everything works ok even with the default build.
t.write("jamfile.jam", """\
exe a : a_empty.cpp : <variant>release ;
exe a : a.cpp : <variant>debug ;
""")

t.run_build_system()
t.expect_addition("bin/$toolset/debug/a.exe")

# Test that only properties which are in the build request matter for
# alternative selection. IOW, alternative with <variant>release is better than
# one with <variant>debug when building the release variant.
t.write("jamfile.jam", """\
exe a : a_empty.cpp : <variant>debug ;
exe a : a.cpp : <variant>release ;
""")

t.run_build_system(["release"])
t.expect_addition("bin/$toolset/release/a.exe")

# Test that free properties do not matter. We really do not want <cxxflags>
# property in build request to affect alternative selection.
t.write("jamfile.jam", """
exe a : a_empty.cpp : <variant>debug <define>FOO <include>BAR ;
exe a : a.cpp : <variant>release ;
""")

t.rm("bin/$toolset/release/a.exe")
t.run_build_system(["release", "define=FOO"])
t.expect_addition("bin/$toolset/release/a.exe")

# Test that ambiguity is reported correctly.
t.write("jamfile.jam", """\
exe a : a_empty.cpp ;
exe a : a.cpp ;
""")
t.run_build_system(["--no-error-backtrace"], status=None)
t.fail_test(string.find(t.stdout(), "No best alternative") == -1)

# Another ambiguity test: two matches properties in one alternative are neither
# better nor worse than a single one in another alternative.
t.write("jamfile.jam", """\
exe a : a_empty.cpp : <optimization>off <profiling>off ;
exe a : a.cpp : <debug-symbols>on ;
""")

t.run_build_system(["--no-error-backtrace"], status=None)
t.fail_test(string.find(t.stdout(), "No best alternative") == -1)

# Test that we can have alternative without sources.
t.write("jamfile.jam", """\
alias specific-sources ;
import feature ;
feature.extend os : MAGIC ;
alias specific-sources : b.cpp : <os>MAGIC ;
exe a : a.cpp specific-sources ;
""")
t.rm("bin")
t.run_build_system()

t.cleanup()

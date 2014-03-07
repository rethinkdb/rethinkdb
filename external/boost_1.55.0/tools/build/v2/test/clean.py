#!/usr/bin/python

#  Copyright (C) Vladimir Prus 2006.
#  Distributed under the Boost Software License, Version 1.0. (See
#  accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("a.cpp", "int main() {}\n")
t.write("jamroot.jam", "exe a : a.cpp sub1//sub1 sub2//sub2 sub3//sub3 ;")
t.write("sub1/jamfile.jam", """\
lib sub1 : sub1.cpp sub1_2 ../sub2//sub2 ;
lib sub1_2 : sub1_2.cpp ;
""")

t.write("sub1/sub1.cpp", """\
#ifdef _WIN32
__declspec(dllexport)
#endif
void sub1() {}
""")

t.write("sub1/sub1_2.cpp", """\
#ifdef _WIN32
__declspec(dllexport)
#endif
void sub1() {}
""")

t.write("sub2/jamfile.jam", "lib sub2 : sub2.cpp ;")
t.write("sub2/sub2.cpp", """\
#ifdef _WIN32
__declspec(dllexport)
#endif
void sub2() {}
""")

t.write("sub3/jamroot.jam", "lib sub3 : sub3.cpp ;")
t.write("sub3/sub3.cpp", """\
#ifdef _WIN32
__declspec(dllexport)
#endif
void sub3() {}
""")

# 'clean' should not remove files under separate jamroot.jam.
t.run_build_system()
t.run_build_system(["--clean"])
t.expect_removal("bin/$toolset/debug/a.obj")
t.expect_removal("sub1/bin/$toolset/debug/sub1.obj")
t.expect_removal("sub1/bin/$toolset/debug/sub1_2.obj")
t.expect_removal("sub2/bin/$toolset/debug/sub2.obj")
t.expect_nothing("sub3/bin/$toolset/debug/sub3.obj")

# 'clean-all' removes everything it can reach.
t.run_build_system()
t.run_build_system(["--clean-all"])
t.expect_removal("bin/$toolset/debug/a.obj")
t.expect_removal("sub1/bin/$toolset/debug/sub1.obj")
t.expect_removal("sub1/bin/$toolset/debug/sub1_2.obj")
t.expect_removal("sub2/bin/$toolset/debug/sub2.obj")
t.expect_nothing("sub3/bin/$toolset/debug/sub3.obj")

# 'clean' together with project target removes only under that project.
t.run_build_system()
t.run_build_system(["sub1", "--clean"])
t.expect_nothing("bin/$toolset/debug/a.obj")
t.expect_removal("sub1/bin/$toolset/debug/sub1.obj")
t.expect_removal("sub1/bin/$toolset/debug/sub1_2.obj")
t.expect_nothing("sub2/bin/$toolset/debug/sub2.obj")
t.expect_nothing("sub3/bin/$toolset/debug/sub3.obj")

# 'clean-all' removes everything.
t.run_build_system()
t.run_build_system(["sub1", "--clean-all"])
t.expect_nothing("bin/$toolset/debug/a.obj")
t.expect_removal("sub1/bin/$toolset/debug/sub1.obj")
t.expect_removal("sub1/bin/$toolset/debug/sub1_2.obj")
t.expect_removal("sub2/bin/$toolset/debug/sub2.obj")
t.expect_nothing("sub3/bin/$toolset/debug/sub3.obj")

# If main target is explicitly named, we should not remove files from other
# targets.
t.run_build_system()
t.run_build_system(["sub1//sub1", "--clean"])
t.expect_removal("sub1/bin/$toolset/debug/sub1.obj")
t.expect_nothing("sub1/bin/$toolset/debug/sub1_2.obj")
t.expect_nothing("sub2/bin/$toolset/debug/sub2.obj")
t.expect_nothing("sub3/bin/$toolset/debug/sub3.obj")

# Regression test: sources of the 'cast' rule were mistakenly deleted.
t.rm(".")
t.write("jamroot.jam", """\
import cast ;
cast a cpp : a.h ;
""")
t.write("a.h", "")
t.run_build_system(["--clean"])
t.expect_nothing("a.h")

t.cleanup()

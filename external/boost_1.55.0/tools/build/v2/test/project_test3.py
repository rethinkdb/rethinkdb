#!/usr/bin/python

# Copyright 2002, 2003 Dave Abrahams
# Copyright 2002, 2003, 2004, 2006 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild
import os

t = BoostBuild.Tester(translate_suffixes=0)

# First check some startup.
t.set_tree("project-test3")
os.remove("jamroot.jam")
t.run_build_system(status=1)

t.expect_output_lines("error: Could not find parent for project at '.'\n"
    "error: Did not find Jamfile.jam or Jamroot.jam in any parent directory.")

t.set_tree("project-test3")
t.run_build_system()

t.expect_addition("bin/$toolset/debug/a.obj")
t.expect_content("bin/$toolset/debug/a.obj", """\
$toolset/debug
a.cpp
""")

t.expect_addition("bin/$toolset/debug/a.exe")
t.expect_content("bin/$toolset/debug/a.exe",
"$toolset/debug\n" +
"bin/$toolset/debug/a.obj lib/bin/$toolset/debug/b.obj " +
"lib2/bin/$toolset/debug/c.obj lib2/bin/$toolset/debug/d.obj " +
"lib2/helper/bin/$toolset/debug/e.obj " +
"lib3/bin/$toolset/debug/f.obj\n"
)

t.expect_addition("lib/bin/$toolset/debug/b.obj")
t.expect_content("lib/bin/$toolset/debug/b.obj", """\
$toolset/debug
lib/b.cpp
""")

t.expect_addition("lib/bin/$toolset/debug/m.exe")
t.expect_content("lib/bin/$toolset/debug/m.exe", """\
$toolset/debug
lib/bin/$toolset/debug/b.obj lib2/bin/$toolset/debug/c.obj
""")

t.expect_addition("lib2/bin/$toolset/debug/c.obj")
t.expect_content("lib2/bin/$toolset/debug/c.obj", """\
$toolset/debug
lib2/c.cpp
""")

t.expect_addition("lib2/bin/$toolset/debug/d.obj")
t.expect_content("lib2/bin/$toolset/debug/d.obj", """\
$toolset/debug
lib2/d.cpp
""")

t.expect_addition("lib2/bin/$toolset/debug/l.exe")
t.expect_content("lib2/bin/$toolset/debug/l.exe", """\
$toolset/debug
lib2/bin/$toolset/debug/c.obj bin/$toolset/debug/a.obj
""")

t.expect_addition("lib2/helper/bin/$toolset/debug/e.obj")
t.expect_content("lib2/helper/bin/$toolset/debug/e.obj", """\
$toolset/debug
lib2/helper/e.cpp
""")

t.expect_addition("lib3/bin/$toolset/debug/f.obj")
t.expect_content("lib3/bin/$toolset/debug/f.obj", """\
$toolset/debug
lib3/f.cpp lib2/helper/bin/$toolset/debug/e.obj
""")

t.touch("a.cpp")
t.run_build_system()
t.expect_touch(["bin/$toolset/debug/a.obj",
                "bin/$toolset/debug/a.exe",
                "lib2/bin/$toolset/debug/l.exe"])

t.run_build_system(["release", "optimization=off,speed"])
t.expect_addition(["bin/$toolset/release/a.exe",
                  "bin/$toolset/release/a.obj",
                  "bin/$toolset/release/optimization-off/a.exe",
                  "bin/$toolset/release/optimization-off/a.obj"])

t.run_build_system(["--clean-all"])
t.expect_removal(["bin/$toolset/debug/a.obj",
                 "bin/$toolset/debug/a.exe",
                 "lib/bin/$toolset/debug/b.obj",
                 "lib/bin/$toolset/debug/m.exe",
                 "lib2/bin/$toolset/debug/c.obj",
                 "lib2/bin/$toolset/debug/d.obj",
                 "lib2/bin/$toolset/debug/l.exe",
                 "lib3/bin/$toolset/debug/f.obj"])

# Now test target ids in command line.
t.set_tree("project-test3")
t.run_build_system(["lib//b.obj"])
t.expect_addition("lib/bin/$toolset/debug/b.obj")
t.expect_nothing_more()

t.run_build_system(["--clean", "lib//b.obj"])
t.expect_removal("lib/bin/$toolset/debug/b.obj")
t.expect_nothing_more()

t.run_build_system(["lib//b.obj"])
t.expect_addition("lib/bin/$toolset/debug/b.obj")
t.expect_nothing_more()

t.run_build_system(["release", "lib2/helper//e.obj", "/lib3//f.obj"])
t.expect_addition("lib2/helper/bin/$toolset/release/e.obj")
t.expect_addition("lib3/bin/$toolset/release/f.obj")
t.expect_nothing_more()

# Test project ids in command line work as well.
t.set_tree("project-test3")
t.run_build_system(["/lib2"])
t.expect_addition("lib2/bin/$toolset/debug/" *
    BoostBuild.List("c.obj d.obj l.exe"))
t.expect_addition("bin/$toolset/debug/a.obj")
t.expect_nothing_more()

t.run_build_system(["lib"])
t.expect_addition("lib/bin/$toolset/debug/" *
    BoostBuild.List("b.obj m.exe"))
t.expect_nothing_more()

t.cleanup()

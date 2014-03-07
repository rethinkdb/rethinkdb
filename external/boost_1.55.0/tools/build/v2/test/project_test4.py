#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2002, 2003, 2004 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

t = BoostBuild.Tester(translate_suffixes=0)


t.set_tree("project-test4")

t.run_build_system()

t.expect_addition("bin/$toolset/debug/a.obj")
t.expect_content("bin/$toolset/debug/a.obj",
"""$toolset/debug/include-everything
a.cpp
""")

t.expect_addition("bin/$toolset/debug/a.exe")
t.expect_content("bin/$toolset/debug/a.exe",
"$toolset/debug/include-everything\n" +
"bin/$toolset/debug/a.obj lib/bin/$toolset/debug/optimization-speed/b.obj\n"
)

t.expect_addition("lib/bin/$toolset/debug/optimization-speed/b.obj")
t.expect_content("lib/bin/$toolset/debug/optimization-speed/b.obj",
"""$toolset/debug/include-everything/optimization-speed
lib/b.cpp
""")

t.expect_addition("bin/$toolset/debug/b.exe")
t.expect_content("bin/$toolset/debug/b.exe",
"$toolset/debug/define-MACROS/include-everything\n" +
"bin/$toolset/debug/a.obj\n"
)

t.copy("lib/jamfile3.jam", "lib/jamfile.jam")

# Link-compatibility check for rtti is disabled...
#t.run_build_system(status=None)
#import string
#t.fail_test(string.find(t.stdout(),
#"""warning: targets produced from b.obj are link incompatible
#warning: with main target a.exe""") !=-0)

# Test that if we specified composite property in target reference, everything
# works OK.

t.copy("lib/jamfile1.jam", "lib/jamfile.jam")
t.copy("jamfile5.jam", "jamfile.jam")

t.run_build_system()

t.expect_addition("lib/bin/$toolset/release/b.obj")

t.expect_content("bin/$toolset/debug/a.exe",
"$toolset/debug/include-everything\n" +
"bin/$toolset/debug/a.obj lib/bin/$toolset/release/b.obj\n"
)

t.cleanup()

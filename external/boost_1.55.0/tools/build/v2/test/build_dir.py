#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2002, 2003, 2005 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test that we can change build directory using the 'build-dir' project
# attribute.

import BoostBuild
import string
import os

t = BoostBuild.Tester(use_test_config=False)


# Test that top-level project can affect build dir.
t.write("jamroot.jam", "import gcc ;")
t.write("jamfile.jam", """\
project : build-dir build ;
exe a : a.cpp ;
build-project src ;
""")

t.write("a.cpp", "int main() {}\n")

t.write("src/jamfile.jam", "exe b : b.cpp ; ")

t.write("src/b.cpp", "int main() {}\n")

t.run_build_system()

t.expect_addition(["build/$toolset/debug/a.exe",
                   "build/src/$toolset/debug/b.exe"])

# Test that building from child projects work.
t.run_build_system(subdir='src')
t.ignore("build/config.log")
t.ignore("build/project-cache.jam")
t.expect_nothing_more()

# Test that project can override build dir.
t.write("jamfile.jam", """\
exe a : a.cpp ;
build-project src ;
""")

t.write("src/jamfile.jam", """\
project : build-dir build ;
exe b : b.cpp ;
""")

t.run_build_system()
t.expect_addition(["bin/$toolset/debug/a.exe",
                   "src/build/$toolset/debug/b.exe"])

# Now test the '--build-dir' option.
t.rm(".")
t.write("jamroot.jam", "")

# Test that we get an error when no project id is specified.
t.run_build_system(["--build-dir=foo"])
t.fail_test(string.find(t.stdout(),
                   "warning: the --build-dir option will be ignored") == -1)

t.write("jamroot.jam", """\
project foo ;
exe a : a.cpp ;
build-project sub ;
""")
t.write("a.cpp", "int main() {}\n")
t.write("sub/jamfile.jam", "exe b : b.cpp ;\n")
t.write("sub/b.cpp", "int main() {}\n")

t.run_build_system(["--build-dir=build"])
t.expect_addition(["build/foo/$toolset/debug/a.exe",
                   "build/foo/sub/$toolset/debug/b.exe"])

t.write("jamroot.jam", """\
project foo : build-dir bin.v2 ;
exe a : a.cpp ;
build-project sub ;
""")

t.run_build_system(["--build-dir=build"])
t.expect_addition(["build/foo/bin.v2/$toolset/debug/a.exe",
                   "build/foo/bin.v2/sub/$toolset/debug/b.exe"])

# Try building in subdir. We expect that the entire build tree with be in
# 'sub/build'. Today, I am not sure if this is what the user expects, but let
# it be.
t.rm('build')
t.run_build_system(["--build-dir=build"], subdir="sub")
t.expect_addition(["sub/build/foo/bin.v2/sub/$toolset/debug/b.exe"])

t.write("jamroot.jam", """\
project foo : build-dir %s ;
exe a : a.cpp ;
build-project sub ;
""" % string.replace(os.getcwd(), '\\', '\\\\'))

t.run_build_system(["--build-dir=build"], status=1)
t.fail_test(string.find(t.stdout(),
    "Absolute directory specified via 'build-dir' project attribute") == -1)

t.cleanup()

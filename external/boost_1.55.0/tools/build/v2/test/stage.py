#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2002, 2003, 2004, 2005, 2006 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test staging.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("jamroot.jam", "")
t.write("jamfile.jam", """\
lib a : a.cpp ;
stage dist : a a.h auxilliary/1 ;
""")
t.write("a.cpp", """\
int
#ifdef _WIN32
__declspec(dllexport)
#endif
must_export_something;
""")
t.write("a.h", "")
t.write("auxilliary/1", "")

t.run_build_system()
t.expect_addition(["dist/a.dll", "dist/a.h", "dist/1"])


# Regression test: the following was causing a "duplicate target name" error.
t.write("jamfile.jam", """\
project : requirements <hardcode-dll-paths>true ;
lib a : a.cpp ;
stage dist : a a.h auxilliary/1 ;
alias dist-alias : dist ;
""")

t.run_build_system()


# Test the <location> property.
t.write("jamfile.jam", """\
lib a : a.cpp ;
stage dist : a : <variant>debug:<location>ds <variant>release:<location>rs ;
""")

t.run_build_system()
t.expect_addition("ds/a.dll")

t.run_build_system(["release"])
t.expect_addition("rs/a.dll")


# Test the <location> property in subprojects. Thanks to Kirill Lapshin for the
# bug report.

t.write("jamroot.jam", "path-constant DIST : dist ;")
t.write("jamfile.jam", "build-project d ;")
t.write("d/jamfile.jam", """\
exe a : a.cpp ;
stage dist : a : <location>$(DIST) ;
""")
t.write("d/a.cpp", "int main() {}\n")

t.run_build_system()
t.expect_addition("dist/a.exe")

t.rm("dist")

# Workaround a BIG BUG: the response file is not deleted, even if application
# *is* deleted. We will try to use the same response file when building from
# subdir, with very bad results.
t.rm("d/bin")
t.run_build_system(subdir="d")
t.expect_addition("dist/a.exe")


# Check that 'stage' does not incorrectly reset target suffixes.
t.write("a.cpp", "int main() {}\n")
t.write("jamroot.jam", """\
import type ;
type.register MYEXE : : EXE ;
type.set-generated-target-suffix MYEXE : <optimization>off : myexe ;
""")

# Since <optimization>off is in properties when 'a' is built and staged, its
# suffix should be "myexe".
t.write("jamfile.jam", """\
stage dist : a ;
myexe a : a.cpp ;
""")

t.run_build_system()
t.expect_addition("dist/a.myexe")

# Test 'stage's ability to traverse dependencies.
t.write("a.cpp", "int main() {}\n")
t.write("l.cpp", """\
void
#if defined(_WIN32)
__declspec(dllexport)
#endif
foo() {}
""")
t.write("jamfile.jam", """\
lib l : l.cpp ;
exe a : a.cpp l ;
stage dist : a : <install-dependencies>on <install-type>EXE <install-type>LIB ;
""")
t.write("jamroot.jam", "")
t.rm("dist")

t.run_build_system()
t.expect_addition("dist/a.exe")
t.expect_addition("dist/l.dll")

# Check that <use> properties are ignored the traversing target for staging.
t.copy("l.cpp", "l2.cpp")
t.copy("l.cpp", "l3.cpp")
t.write("jamfile.jam", """\
lib l2 : l2.cpp ;
lib l3 : l3.cpp ;
lib l : l.cpp : <use>l2 <dependency>l3 ;
exe a : a.cpp l ;
stage dist : a : <install-dependencies>on <install-type>EXE <install-type>LIB ;
""")
t.rm("dist")

t.run_build_system()
t.expect_addition("dist/l3.dll")
t.expect_nothing("dist/l2.dll")

# Check if <dependency> on 'stage' works.
t.rm(".")
t.write("jamroot.jam", """\
stage a1 : a1.txt : <location>dist ;
stage a2 : a2.txt : <location>dist <dependency>a1 ;
""")
t.write("a1.txt", "")
t.write("a2.txt", "")
t.run_build_system(["a2"])
t.expect_addition(["dist/a1.txt", "dist/a2.txt"])

# Regression test: check that <location>. works.
t.rm(".")
t.write("jamroot.jam", "stage a1 : d/a1.txt : <location>. ;")
t.write("d/a1.txt", "")

t.run_build_system()
t.expect_addition("a1.txt")

# Test that relative paths of sources can be preserved.
t.rm(".")
t.write("jamroot.jam", "install dist : a/b/c.h : <install-source-root>. ;")
t.write("a/b/c.h", "")

t.run_build_system()
t.expect_addition("dist/a/b/c.h")

t.write("jamroot.jam", "install dist : a/b/c.h : <install-source-root>a ;")
t.write("a/b/c.h", "")

t.run_build_system()
t.expect_addition("dist/b/c.h")

t.rm(".")
t.write("build/jamroot.jam", """\
install dist : ../a/b/c.h : <location>../dist <install-source-root>../a ;
""")
t.write("a/b/c.h", "")

t.run_build_system(subdir="build")
t.expect_addition("dist/b/c.h")

t.write("jamroot.jam", "install dist2 : a/b/c.h : <install-source-root>a ;")
t.write("a/b/c.h", "")
t.write("sub/jamfile.jam", "alias h : ..//dist2 ;")

t.run_build_system(subdir="sub")
t.expect_addition("dist2/b/c.h")

# Test that when installing .cpp files, we do not scan include dependencies.
t.rm(".")
t.write("jamroot.jam", "install dist : a.cpp ;")
t.write("a.cpp", '#include "a.h"')
t.write("a.h", "")

t.run_build_system()
t.expect_addition("dist/a.cpp")

t.touch("a.h")

t.run_build_system()
t.expect_nothing("dist/a.cpp")

# Test that <name> property works, when there is just one file in sources.
t.rm(".")
t.write("jamroot.jam", "install dist : a.cpp : <name>b.cpp ;")
t.write("a.cpp", "test file")

t.run_build_system()
t.expect_addition("dist/b.cpp")

t.cleanup()

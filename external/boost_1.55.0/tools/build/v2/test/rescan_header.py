#!/usr/bin/python

# Copyright 2012 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

# Test a header loop that depends on (but does not contain) a generated header.
t.write("test.cpp", '#include "header1.h"\n')

t.write("header1.h", """\
#ifndef HEADER1_H
#define HEADER1_H
#include "header2.h"
#endif
""")

t.write("header2.h", """\
#ifndef HEADER2_H
#define HEADER2_H
#include "header1.h"
#include "header3.h"
#endif
""")

t.write("header3.in", "/* empty file */\n")

t.write("jamroot.jam", """\
import common ;
make header3.h : header3.in : @common.copy ;
obj test : test.cpp : <implicit-dependency>header3.h ;
""")

t.run_build_system(["-j2"])
t.expect_addition("bin/$toolset/debug/header3.h")
t.expect_addition("bin/$toolset/debug/test.obj")
t.expect_nothing_more()

t.rm(".")

# Test a linear sequence of generated headers.
t.write("test.cpp", '#include "header1.h"\n')

t.write("header1.in", """\
#ifndef HEADER1_H
#define HEADER1_H
#include "header2.h"
#endif
""")

t.write("header2.in", """\
#ifndef HEADER2_H
#define HEADER2_H
#include "header3.h"
#endif
""")

t.write("header3.in", "/* empty file */\n")

t.write("jamroot.jam", """\
import common ;
make header1.h : header1.in : @common.copy ;
make header2.h : header2.in : @common.copy ;
make header3.h : header3.in : @common.copy ;
obj test : test.cpp :
    <implicit-dependency>header1.h
    <implicit-dependency>header2.h
    <implicit-dependency>header3.h ;
""")

t.run_build_system(["-j2", "test"])
t.expect_addition("bin/$toolset/debug/header1.h")
t.expect_addition("bin/$toolset/debug/header2.h")
t.expect_addition("bin/$toolset/debug/header3.h")
t.expect_addition("bin/$toolset/debug/test.obj")
t.expect_nothing_more()

t.rm(".")

# Test a loop in generated headers.
t.write("test.cpp", '#include "header1.h"\n')

t.write("header1.in", """\
#ifndef HEADER1_H
#define HEADER1_H
#include "header2.h"
#endif
""")

t.write("header2.in", """\
#ifndef HEADER2_H
#define HEADER2_H
#include "header3.h"
#endif
""")

t.write("header3.in", """\
#ifndef HEADER2_H
#define HEADER2_H
#include "header1.h"
#endif
""")

t.write("jamroot.jam", """\
import common ;

actions copy {
    sleep 1
    cp $(>) $(<)
}

make header1.h : header1.in : @common.copy ;
make header2.h : header2.in : @common.copy ;
make header3.h : header3.in : @common.copy ;
obj test : test.cpp :
    <implicit-dependency>header1.h
    <implicit-dependency>header2.h
    <implicit-dependency>header3.h ;
""")

t.run_build_system(["-j2", "test"])
t.expect_addition("bin/$toolset/debug/header1.h")
t.expect_addition("bin/$toolset/debug/header2.h")
t.expect_addition("bin/$toolset/debug/header3.h")
t.expect_addition("bin/$toolset/debug/test.obj")
t.expect_nothing_more()

t.rm(".")

# Test that all the dependencies of a loop are updated before any of the
# dependents.
t.write("test1.cpp", '#include "header1.h"\n')

t.write("test2.cpp", """\
#include "header2.h"
int main() {}
""")

t.write("header1.h", """\
#ifndef HEADER1_H
#define HEADER1_H
#include "header2.h"
#endif
""")

t.write("header2.h", """\
#ifndef HEADER2_H
#define HEADER2_H
#include "header1.h"
#include "header3.h"
#endif
""")

t.write("header3.in", "\n")

t.write("sleep.bat", """\
::@timeout /T %1 /NOBREAK >nul
@ping 127.0.0.1 -n 2 -w 1000 >nul
@ping 127.0.0.1 -n %1 -w 1000 >nul
@exit /B 0
""")

t.write("jamroot.jam", """\
import common ;
import os ;

if [ os.name ] = NT
{
    SLEEP = call sleep.bat ;
}
else
{
    SLEEP = sleep ;
}

rule copy { common.copy $(<) : $(>) ; }
actions copy { $(SLEEP) 1 }

make header3.h : header3.in : @copy ;
exe test : test2.cpp test1.cpp : <implicit-dependency>header3.h ;
""")

t.run_build_system(["-j2", "test"])
t.expect_addition("bin/$toolset/debug/header3.h")
t.expect_addition("bin/$toolset/debug/test1.obj")
t.expect_addition("bin/$toolset/debug/test2.obj")
t.expect_addition("bin/$toolset/debug/test.exe")
t.expect_nothing_more()

t.touch("header3.in")
t.run_build_system(["-j2", "test"])
t.expect_touch("bin/$toolset/debug/header3.h")
t.expect_touch("bin/$toolset/debug/test1.obj")
t.expect_touch("bin/$toolset/debug/test2.obj")
t.expect_touch("bin/$toolset/debug/test.exe")
t.expect_nothing_more()

t.rm(".")

# Test a loop that includes a generated header
t.write("test1.cpp", '#include "header1.h"\n')
t.write("test2.cpp", """\
#include "header2.h"
int main() {}
""")

t.write("header1.h", """\
#ifndef HEADER1_H
#define HEADER1_H
#include "header2.h"
#endif
""")

t.write("header2.in", """\
#ifndef HEADER2_H
#define HEADER2_H
#include "header3.h"
#endif
""")

t.write("header3.h", """\
#ifndef HEADER3_H
#define HEADER3_H
#include "header1.h"
#endif
""")

t.write("sleep.bat", """\
::@timeout /T %1 /NOBREAK >nul
@ping 127.0.0.1 -n 2 -w 1000 >nul
@ping 127.0.0.1 -n %1 -w 1000 >nul
@exit /B 0
""")

t.write("jamroot.jam", """\
import common ;
import os ;

if [ os.name ] = NT
{
    SLEEP = call sleep.bat ;
}
else
{
    SLEEP = sleep ;
}

rule copy { common.copy $(<) : $(>) ; }
actions copy { $(SLEEP) 1 }

make header2.h : header2.in : @copy ;
exe test : test2.cpp test1.cpp : <implicit-dependency>header2.h <include>. ;
""")

t.run_build_system(["-j2", "test"])
t.expect_addition("bin/$toolset/debug/header2.h")
t.expect_addition("bin/$toolset/debug/test1.obj")
t.expect_addition("bin/$toolset/debug/test2.obj")
t.expect_addition("bin/$toolset/debug/test.exe")
t.expect_nothing_more()

t.cleanup()

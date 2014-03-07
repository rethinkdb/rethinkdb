#!/usr/bin/python

# Copyright 2007 Rene Rivera.
# Copyright 2011 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

t = BoostBuild.Tester(pass_toolset=0)

t.write("file.jam", """\
actions quietly .a.
{
echo [$(<:B)] 0
echo [$(<:B)] 1
echo [$(<:B)] 2
}

rule .a.
{
    DEPENDS $(<) : $(>) ;
}

NOTFILE subtest ;
.a. subtest_a : subtest ;
.a. subtest_b : subtest ;
DEPENDS all : subtest_a subtest_b ;
""")

t.run_build_system(["-ffile.jam", "-d2"], stdout="""\
...found 4 targets...
...updating 2 targets...
.a. subtest_a

echo [subtest_a] 0
echo [subtest_a] 1
echo [subtest_a] 2

[subtest_a] 0
[subtest_a] 1
[subtest_a] 2
.a. subtest_b

echo [subtest_b] 0
echo [subtest_b] 1
echo [subtest_b] 2

[subtest_b] 0
[subtest_b] 1
[subtest_b] 2
...updated 2 targets...
""")

t.run_build_system(["-ffile.jam", "-d1"], stdout="""\
...found 4 targets...
...updating 2 targets...
...updated 2 targets...
""")

t.cleanup()

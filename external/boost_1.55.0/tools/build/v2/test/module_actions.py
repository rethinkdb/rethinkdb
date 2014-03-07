#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2006 Rene Rivera
# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Demonstration that module variables have the correct effect in actions.

import BoostBuild
import os
import re

t = BoostBuild.Tester(["-d+1"], pass_toolset=0)

t.write("boost-build.jam", "boost-build . ;")
t.write("bootstrap.jam", """\
# Top-level rule causing a target to be built by invoking the specified action.
rule make ( target : sources * : act )
{
    DEPENDS all : $(target) ;
    DEPENDS $(target) : $(sources) ;
    $(act) $(target) : $(sources) ;
}

X1 = X1-global ;
X2 = X2-global ;
X3 = X3-global ;

module A
{
    X1 = X1-A ;

    rule act ( target )
    {
        NOTFILE $(target) ;
        ALWAYS $(target) ;
    }

    actions act { echo A.act $(<): $(X1) $(X2) $(X3) }

    make t1 : : A.act ;
    make t2 : : A.act ;
    make t3 : : A.act ;
}

module B
{
    X2 = X2-B ;

    actions act { echo B.act $(<): $(X1) $(X2) $(X3) }

    make t1 : : B.act ;
    make t2 : : B.act ;
    make t3 : : B.act ;
}

actions act { echo act $(<): $(X1) $(X2) $(X3) }

make t1 : : act ;
make t2 : : act ;
make t3 : : act ;

X1 on t1 = X1-t1 ;
X2 on t2 = X2-t2 ;
X3 on t3 = X3-t3 ;

DEPENDS all : t1 t2 t3 ;
""")

expected_lines = [
    "...found 4 targets...",
    "...updating 3 targets...",
    "A.act t1",
    "A.act t1: X1-t1   ",
    "B.act t1",
    "B.act t1: X1-t1 X2-B  ",
    "act t1",
    "act t1: X1-t1 X2-global X3-global ",
    "A.act t2",
    "A.act t2: X1-A X2-t2  ",
    "B.act t2",
    "B.act t2:  X2-t2  ",
    "act t2",
    "act t2: X1-global X2-t2 X3-global ",
    "A.act t3",
    "A.act t3: X1-A  X3-t3 ",
    "B.act t3",
    "B.act t3:  X2-B X3-t3 ",
    "act t3",
    "act t3: X1-global X2-global X3-t3 ",
    "...updated 3 targets...",
    ""]

# Accommodate for the fact that on Unixes, a call to 'echo 1   2     3  '
# produces '1 2 3' (note the spacing).
if os.name != 'nt':
    expected_lines = [re.sub("  +", " ", x.rstrip()) for x in expected_lines]

t.run_build_system()
t.expect_output_lines(expected_lines)
t.expect_nothing_more()
t.cleanup()

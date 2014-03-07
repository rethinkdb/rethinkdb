#!/usr/bin/python

# Copyright 2011 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild
import os


def basic():
    t = BoostBuild.Tester(pass_toolset=0, pass_d0=False)

    t.write("file.jam", """\
actions do-print
{
    echo updating $(<)
}

NOTFILE target1 ;
ALWAYS target1 ;
do-print target1 ;

UPDATE_NOW target1 ;

DEPENDS all : target1 ;
""")

    t.run_build_system(["-ffile.jam"], stdout="""\
...found 1 target...
...updating 1 target...
do-print target1
updating target1
...updated 1 target...
...found 1 target...
""")

    t.cleanup()


def ignore_minus_n():
    t = BoostBuild.Tester(pass_toolset=0, pass_d0=False)

    t.write("file.jam", """\
actions do-print
{
    echo updating $(<)
}

NOTFILE target1 ;
ALWAYS target1 ;
do-print target1 ;

UPDATE_NOW target1 : : ignore-minus-n ;

DEPENDS all : target1 ;
""")

    t.run_build_system(["-ffile.jam", "-n"], stdout="""\
...found 1 target...
...updating 1 target...
do-print target1

    echo updating target1

updating target1
...updated 1 target...
...found 1 target...
""")

    t.cleanup()


def failed_target():
    t = BoostBuild.Tester(pass_toolset=0, pass_d0=False)

    t.write("file.jam", """\
actions fail
{
    exit 1
}

NOTFILE target1 ;
ALWAYS target1 ;
fail target1 ;

actions do-print
{
    echo updating $(<)
}

NOTFILE target2 ;
do-print target2 ;
DEPENDS target2 : target1 ;

UPDATE_NOW target1 : : ignore-minus-n ;

DEPENDS all : target1 target2 ;
""")

    t.run_build_system(["-ffile.jam", "-n"], stdout="""\
...found 1 target...
...updating 1 target...
fail target1

    exit 1

...failed fail target1...
...failed updating 1 target...
...found 2 targets...
...updating 1 target...
do-print target2

    echo updating target2

...updated 1 target...
""")

    t.cleanup()


def missing_target():
    t = BoostBuild.Tester(pass_toolset=0, pass_d0=False)

    t.write("file.jam", """\
actions do-print
{
    echo updating $(<)
}

NOTFILE target2 ;
do-print target2 ;
DEPENDS target2 : target1 ;

UPDATE_NOW target1 : : ignore-minus-n ;

DEPENDS all : target1 target2 ;
""")

    t.run_build_system(["-ffile.jam", "-n"], status=1, stdout="""\
don't know how to make target1
...found 1 target...
...can't find 1 target...
...found 2 targets...
...can't make 1 target...
""")

    t.cleanup()


def build_once():
    """
      Make sure that if we call UPDATE_NOW with ignore-minus-n, the target gets
    updated exactly once regardless of previous calls to UPDATE_NOW with -n in
    effect.

    """
    t = BoostBuild.Tester(pass_toolset=0, pass_d0=False)

    t.write("file.jam", """\
actions do-print
{
    echo updating $(<)
}

NOTFILE target1 ;
ALWAYS target1 ;
do-print target1 ;

UPDATE_NOW target1 ;
UPDATE_NOW target1 : : ignore-minus-n ;
UPDATE_NOW target1 : : ignore-minus-n ;

DEPENDS all : target1 ;
""")

    t.run_build_system(["-ffile.jam", "-n"], stdout="""\
...found 1 target...
...updating 1 target...
do-print target1

    echo updating target1

...updated 1 target...
do-print target1

    echo updating target1

updating target1
...updated 1 target...
...found 1 target...
""")

    t.cleanup()


def return_status():
    """
    Make sure that UPDATE_NOW returns a failure status if
    the target failed in a previous call to UPDATE_NOW
    """
    t = BoostBuild.Tester(pass_toolset=0, pass_d0=False)

    t.write("file.jam", """\
actions fail
{
    exit 1
}

NOTFILE target1 ;
ALWAYS target1 ;
fail target1 ;

ECHO update1: [ UPDATE_NOW target1 ] ;
ECHO update2: [ UPDATE_NOW target1 ] ;

DEPENDS all : target1 ;
""")

    t.run_build_system(["-ffile.jam"], status=1, stdout="""\
...found 1 target...
...updating 1 target...
fail target1

    exit 1

...failed fail target1...
...failed updating 1 target...
update1:
update2:
...found 1 target...
""")

    t.cleanup()


def save_restore():
    """Tests that ignore-minus-n and ignore-minus-q are
    local to the call to UPDATE_NOW"""
    t = BoostBuild.Tester(pass_toolset=0, pass_d0=False)

    t.write("actions.jam", """\
rule fail
{
    NOTFILE $(<) ;
    ALWAYS $(<) ;
}
actions fail
{
    exit 1
}

rule pass
{
    NOTFILE $(<) ;
    ALWAYS $(<) ;
}
actions pass
{
    echo updating $(<)
}
""")
    t.write("file.jam", """
include actions.jam ;
fail target1 ;
fail target2 ;
UPDATE_NOW target1 target2 : : $(IGNORE_MINUS_N) : $(IGNORE_MINUS_Q) ;
fail target3 ;
fail target4 ;
UPDATE_NOW target3 target4 ;
UPDATE ;
""")
    t.run_build_system(['-n', '-sIGNORE_MINUS_N=1', '-ffile.jam'],
                       stdout='''...found 2 targets...
...updating 2 targets...
fail target1

    exit 1

...failed fail target1...
fail target2

    exit 1

...failed fail target2...
...failed updating 2 targets...
...found 2 targets...
...updating 2 targets...
fail target3

    exit 1

fail target4

    exit 1

...updated 2 targets...
''')

    t.run_build_system(['-q', '-sIGNORE_MINUS_N=1', '-ffile.jam'],
                       status=1, stdout='''...found 2 targets...
...updating 2 targets...
fail target1

    exit 1

...failed fail target1...
...failed updating 1 target...
...found 2 targets...
...updating 2 targets...
fail target3

    exit 1

...failed fail target3...
...failed updating 1 target...
''')

    t.run_build_system(['-n', '-sIGNORE_MINUS_Q=1', '-ffile.jam'],
                       stdout='''...found 2 targets...
...updating 2 targets...
fail target1

    exit 1

fail target2

    exit 1

...updated 2 targets...
...found 2 targets...
...updating 2 targets...
fail target3

    exit 1

fail target4

    exit 1

...updated 2 targets...
''')

    t.run_build_system(['-q', '-sIGNORE_MINUS_Q=1', '-ffile.jam'],
                       status=1, stdout='''...found 2 targets...
...updating 2 targets...
fail target1

    exit 1

...failed fail target1...
fail target2

    exit 1

...failed fail target2...
...failed updating 2 targets...
...found 2 targets...
...updating 2 targets...
fail target3

    exit 1

...failed fail target3...
...failed updating 1 target...
''')

    t.cleanup()


basic()
ignore_minus_n()
failed_target()
missing_target()
build_once()
return_status()
save_restore()

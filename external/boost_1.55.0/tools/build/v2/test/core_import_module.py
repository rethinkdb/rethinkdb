#!/usr/bin/python

# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

t = BoostBuild.Tester(pass_toolset=0)

t.write("code", """\
module a
{
    rule r1 ( )
    {
        ECHO R1 ;
    }

    local rule l1 ( )
    {
        ECHO A.L1 ;
    }
}
module a2
{
    rule r2 ( )
    {
        ECHO R2 ;
    }
}
IMPORT a2 : r2 : : a2.r2 ;

rule a.l1 ( )
{
    ECHO L1 ;
}

module b
{
    IMPORT_MODULE a : b ;
    rule test
    {
        # Call rule visible via IMPORT_MODULE
        a.r1 ;
        # Call rule in global scope
        a2.r2 ;
        # Call rule in global scope.  Doesn't find local rule
        a.l1 ;
        # Make l1 visible
        EXPORT a : l1 ;
        a.l1 ;
    }
}

IMPORT b : test : : test ;
test ;

module c
{
    rule test
    {
        ECHO CTEST ;
    }
}

IMPORT_MODULE c : ;
c.test ;

actions do-nothing { }
do-nothing all ;
""")

t.run_build_system(["-fcode"], stdout="""\
R1
R2
L1
A.L1
CTEST
""")

t.cleanup()

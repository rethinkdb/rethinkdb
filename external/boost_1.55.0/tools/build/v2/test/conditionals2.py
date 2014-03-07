#!/usr/bin/python

# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Regression test: it was possible that due to evaluation of conditional
# requirements, two different values of non-free features were present in a
# property set.

import BoostBuild

t = BoostBuild.Tester()

t.write("a.cpp", "")

t.write("jamroot.jam", """
import feature ;
import common ;

feature.feature the_feature : false true : propagated ;

rule maker ( targets * : sources * : properties * )
{
    if <the_feature>false in $(properties) &&
        <the_feature>true in $(properties)
    {
        EXIT "Oops, two different values of non-free feature" ;
    }
    CMD on $(targets) = [ common.file-creation-command ] ;
}

actions maker
{
    $(CMD) $(<) ;
}

make a : a.cpp : maker : <variant>debug:<the_feature>true ;
""")

t.run_build_system()

t.cleanup()

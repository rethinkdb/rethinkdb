#!/usr/bin/python

# Copyright 2001 Dave Abrahams 
# Copyright 2011 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 

import BoostBuild
import os

t = BoostBuild.Tester(pass_toolset=0, pass_d0=False)

t.write("subdir1/file-to-bind", "# This file intentionally left blank")

t.write("file.jam", """\
rule do-nothing ( target : source )
{
    DEPENDS $(target) : $(source) ;
}
actions quietly do-nothing { }

# Make a non-file target which depends on a file that exists
NOTFILE fake-target ;
SEARCH on file-to-bind = subdir1 ;

do-nothing fake-target : file-to-bind ;

# Set jam up to call our bind-rule
BINDRULE = bind-rule ;

rule bind-rule ( target : path )
{
    ECHO found: $(target) at $(path) ;
}

DEPENDS all : fake-target ;
""")

t.run_build_system(["-ffile.jam"], stdout="""\
found: all at all
found: file-to-bind at subdir1%sfile-to-bind
...found 3 targets...
""" % os.sep)

t.cleanup()

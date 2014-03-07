#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# This tests the facilities for deleting modules.

import BoostBuild

t = BoostBuild.Tester(pass_toolset=0)

t.write("file.jam", """
module foo
{
    rule bar { }
    var = x y ;
}
DELETE_MODULE foo ;
if [ RULENAMES foo ]
{
     EXIT DELETE_MODULE failed to kill foo's rules: [ RULENAMES foo ] ;
}

module foo
{
     if $(var)
     {
         EXIT DELETE_MODULE failed to kill foo's variables ;
     }

     rule bar { }
     var = x y ;

     DELETE_MODULE foo ;

     if $(var)
     {
         EXIT internal DELETE_MODULE failed to kill foo's variables ;
     }
     if [ RULENAMES foo ]
     {
         EXIT internal DELETE_MODULE failed to kill foo's rules: [ RULENAMES foo ] ;
     }
}
DEPENDS all : xx ;
NOTFILE xx ;
""")

t.run_build_system(["-ffile.jam"], status=0)
t.cleanup()

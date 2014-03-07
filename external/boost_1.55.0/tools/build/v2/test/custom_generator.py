#!/usr/bin/python

# Copyright 2003, 2004, 2005 Vladimir Prus 
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 

# Attempt to declare a generator for creating OBJ from RC files. That generator
# should be considered together with standard CPP->OBJ generators and
# successfully create the target. Since we do not have a RC compiler everywhere,
# we fake the action. The resulting OBJ will be unusable, but it must be
# created.

import BoostBuild

t = BoostBuild.Tester()

t.write("jamroot.jam", """ 
import rcc ; 
""")

t.write("rcc.jam", """ 
import type ;
import generators ;
import print ;

# Use 'RCC' to avoid conflicts with definitions in the standard rc.jam and
# msvc.jam
type.register RCC : rcc ;

rule resource-compile ( targets * : sources * : properties * )
{
    print.output $(targets[1]) ;
    print.text "rc-object" ;
}

generators.register-standard rcc.resource-compile : RCC : OBJ ;
""")

t.write("rcc.py", """
import b2.build.type as type
import b2.build.generators as generators

from b2.manager import get_manager

# Use 'RCC' to avoid conflicts with definitions in the standard rc.jam and
# msvc.jam
type.register('RCC', ['rcc'])

generators.register_standard("rcc.resource-compile", ["RCC"], ["OBJ"])

get_manager().engine().register_action(
    "rcc.resource-compile",
    '@($(STDOUT):E=rc-object) > "$(<)"')
""")

t.write("jamfile.jam", """ 
obj r : r.rcc ; 
""")

t.write("r.rcc", """ 
""")

t.run_build_system()
t.expect_content("bin/$toolset/debug/r.obj", "rc-object")

t.cleanup()

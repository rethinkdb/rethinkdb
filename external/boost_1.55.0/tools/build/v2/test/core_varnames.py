#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# This tests the core rule for enumerating the variable names in a module.

import BoostBuild

t = BoostBuild.Tester(pass_toolset=0)

t.write("file.jam", """\
module foo
{
    rule bar { }
    var1 = x y ;
    var2 = fubar ;
}

expected = var1 var2 ;
names = [ VARNAMES foo ] ;
if $(names) in $(expected) && $(expected) in $(names)
{
    # everything OK
}
else
{
    EXIT expected to find variables $(expected:J=", ") in module foo,
    but found $(names:J=", ") instead. ;
}
DEPENDS all : xx ;
NOTFILE xx ;
""")

t.run_build_system(["-ffile.jam"], status=0)

t.cleanup()

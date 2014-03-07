#!/usr/bin/python

# Copyright 2003, 2004, 2005 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

# Create a temporary working directory.
t = BoostBuild.Tester()

# Create the needed files.
t.write("jamroot.jam", """\
constant FOO : foobar gee ;
ECHO $(FOO) ;
""")

t.run_build_system()
t.expect_output_lines("foobar gee")

# Regression test: when absolute paths were passed to path-constant rule,
# Boost.Build failed to recognize path as absolute and prepended the current
# dir.
t.write("jamroot.jam", """\
import path ;
local here = [ path.native [ path.pwd ] ] ;
path-constant HERE : $(here) ;
if $(HERE) != $(here)
{
    ECHO "PWD           =" $(here) ;
    ECHO "path constant =" $(HERE) ;
    EXIT ;
}
""")
t.write("jamfile.jam", "")

t.run_build_system()

t.write("jamfile.jam", """\
# This tests that rule 'hello' will be imported to children unlocalized, and
# will still access variables in this Jamfile.
x = 10 ;
constant FOO : foo ;
rule hello ( ) { ECHO "Hello $(x)" ; }
""")

t.write("d/jamfile.jam", """\
ECHO "d: $(FOO)" ;
constant BAR : bar ;
""")

t.write("d/d2/jamfile.jam", """\
ECHO "d2: $(FOO)" ;
ECHO "d2: $(BAR)" ;
hello ;
""")

t.run_build_system(subdir="d/d2")
t.expect_output_lines("d: foo\nd2: foo\nd2: bar\nHello 10")

t.cleanup()

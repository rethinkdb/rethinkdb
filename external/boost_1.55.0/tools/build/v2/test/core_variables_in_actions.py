#!/usr/bin/python

# Copyright 2012. Jurko Gospodnetic
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

#   Tests that variables in actions get expanded but double quote characters
# get treated as regular characters and not string literal delimiters when
# determining string tokens concatenated to the variable being expanded.
#
#   We also take care to make this test work correctly when run using both
# Windows and Unix echo command variant. That is why we add the extra single
# quotes around the text being echoed - they will make the double quotes be
# displayed as regular characters in both cases but will be displayed
# themselves only when using the Windows cmd shell's echo command.

import BoostBuild

t = BoostBuild.Tester(pass_toolset=0)
t.write("file.jam", """\
rule dummy ( i )
{
    local a = 1 2 3 ;
    ECHO From rule: $(a)" seconds" ;
    a on $(i) = $(a) ;
}

actions dummy
{
    echo 'From action: $(a)" seconds"'
}

dummy all ;
""")
t.run_build_system(["-ffile.jam", "-d1"])
t.expect_output_lines("From rule: 1 seconds 2 seconds 3 seconds")
t.expect_output_lines('*From action: 1" 2" 3" seconds"*')
t.cleanup()

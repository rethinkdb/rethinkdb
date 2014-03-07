#!/usr/bin/python

# Copyright 2007 Rene Rivera.
# Copyright 2011 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

t = BoostBuild.Tester(pass_toolset=0)

t.write("sleep.bat", """\
::@timeout /T %1 /NOBREAK >nul
@ping 127.0.0.1 -n 2 -w 1000 >nul
@ping 127.0.0.1 -n %1 -w 1000 >nul
@exit /B 0
""")

t.write("file.jam", """\
if $(NT)
{
    SLEEP = @call sleep.bat ;
}
else
{
    SLEEP = sleep ;
}

actions .a. {
echo 001
$(SLEEP) 4
echo 002
}

.a. sleeper ;

DEPENDS all : sleeper ;
""")

t.run_build_system(["-ffile.jam", "-d1", "-l2"], status=1)
t.expect_output_lines("2 second time limit exceeded")

t.cleanup()

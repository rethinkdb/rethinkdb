#!/usr/bin/python

# Copyright 2006 Rene Rivera.
# Copyright 2011 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

t = BoostBuild.Tester(pass_toolset=0, pass_d0=False)

t.write("sleep.bat", """\
::@timeout /T %1 /NOBREAK >nul
@ping 127.0.0.1 -n 2 -w 1000 >nul
@ping 127.0.0.1 -n %1 -w 1000 >nul
@exit /B 0
""")

t.write("file.jam", """\
if $(NT)
{
    actions sleeper
    {
        echo [$(<:S)] 0
        call sleep.bat 1
        echo [$(<:S)] 1
        call sleep.bat 1
        echo [$(<:S)] 2
        call sleep.bat $(<:B)
    }
}
else
{
    actions sleeper
    {
        echo "[$(<:S)] 0"
        sleep 1
        echo "[$(<:S)] 1"
        sleep 1
        echo "[$(<:S)] 2"
        sleep $(<:B)
    }
}

rule sleeper
{
    DEPENDS $(<) : $(>) ;
}

NOTFILE front ;
sleeper 1.a : front ;
sleeper 2.a : front ;
sleeper 3.a : front ;
sleeper 4.a : front ;
NOTFILE choke ;
DEPENDS choke : 1.a 2.a 3.a 4.a ;
sleeper 1.b : choke ;
sleeper 2.b : choke ;
sleeper 3.b : choke ;
sleeper 4.b : choke ;
DEPENDS bottom : 1.b 2.b 3.b 4.b ;
DEPENDS all : bottom ;
""")

t.run_build_system(["-ffile.jam", "-j4"], stdout="""\
...found 12 targets...
...updating 8 targets...
sleeper 1.a
[.a] 0
[.a] 1
[.a] 2
sleeper 2.a
[.a] 0
[.a] 1
[.a] 2
sleeper 3.a
[.a] 0
[.a] 1
[.a] 2
sleeper 4.a
[.a] 0
[.a] 1
[.a] 2
sleeper 1.b
[.b] 0
[.b] 1
[.b] 2
sleeper 2.b
[.b] 0
[.b] 1
[.b] 2
sleeper 3.b
[.b] 0
[.b] 1
[.b] 2
sleeper 4.b
[.b] 0
[.b] 1
[.b] 2
...updated 8 targets...
""")

t.cleanup()

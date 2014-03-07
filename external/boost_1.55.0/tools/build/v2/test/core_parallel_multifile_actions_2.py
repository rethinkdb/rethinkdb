#!/usr/bin/python

# Copyright 2008 Jurko Gospodnetic, Vladimir Prus
# Copyright 2011 Steven Watanabe
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#   Added to guard against a bug causing targets to be used before they
# themselves have finished building. This used to happen for targets built by a
# multi-file action that got triggered by another target, except when the
# target triggering the action was the first one in the list of targets
# produced by that action.
#
# Example:
#   When target A and target B were declared as created by a single action with
# A being the first one listed, and target B triggered running that action
# then, while the action was still running, target A was already reporting as
# being built causing other targets depending on target A to be built
# prematurely.

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
    SLEEP = @call sleep.bat ;
}
else
{
    SLEEP = sleep ;
}

actions link
{
    $(SLEEP) 1
    echo 001 - linked
}

link dll lib ;

actions install
{
    echo 002 - installed
}

install installed_dll : dll ;
DEPENDS installed_dll : dll ;

DEPENDS all : lib installed_dll ;
""")

t.run_build_system(["-ffile.jam", "-j2"], stdout="""\
...found 4 targets...
...updating 3 targets...
link dll
001 - linked
install installed_dll
002 - installed
...updated 3 targets...
""")

t.cleanup()

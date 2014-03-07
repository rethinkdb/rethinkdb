#!/usr/bin/python

# Copyright (C) 2012. Jurko Gospodnetic
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Tests Boost Build's project-id handling.

import BoostBuild
import sys


def test_assigning_project_ids():
    t = BoostBuild.Tester(pass_toolset=False)
    t.write("jamroot.jam", """\
import assert ;
import modules ;
import notfile ;
import project ;

rule assert-project-id ( id ? : module-name ? )
{
    module-name ?= [ CALLER_MODULE ] ;
    assert.result $(id) : project.attribute $(module-name) id ;
}

# Project rule modifies the main project id.
assert-project-id ;  # Initial project id is empty
project foo  ; assert-project-id /foo ;
project      ; assert-project-id /foo ;
project foo  ; assert-project-id /foo ;
project bar  ; assert-project-id /bar ;
project /foo ; assert-project-id /foo ;
project ""   ; assert-project-id /foo ;

# Calling the use-project rule does not modify the project's main id.
use-project id1 : a ;
# We need to load the 'a' Jamfile module manually as the use-project rule will
# only schedule the load to be done after the current module load finishes.
a-module = [ project.load a ] ;
assert-project-id : $(a-module) ;
use-project id2 : a ;
assert-project-id : $(a-module) ;
modules.call-in $(a-module) : project baz ;
assert-project-id /baz : $(a-module) ;
use-project id3 : a ;
assert-project-id /baz : $(a-module) ;

# Make sure the project id still holds after all the scheduled use-project loads
# complete. We do this by scheduling the assert for the Jam action scheduling
# phase.
notfile x : @assert-a-rule ;
rule assert-a-rule ( target : : properties * )
{
    assert-project-id /baz : $(a-module) ;
}
""")
    t.write("a/jamfile.jam", """\
# Initial project id for this module is empty.
assert-project-id ;
""")
    t.run_build_system()
    t.cleanup()


def test_using_project_ids_in_target_references():
    t = BoostBuild.Tester()
    __write_appender(t, "appender.jam")
    t.write("jamroot.jam", """\
import type ;
type.register AAA : _a ;
type.register BBB : _b ;

import appender ;
appender.register aaa-to-bbb : AAA : BBB ;

use-project id1 : a ;
use-project /id2 : a ;

bbb b1 : /id1//target ;
bbb b2 : /id2//target ;
bbb b3 : /id3//target ;
bbb b4 : a//target ;
bbb b5 : /project-a1//target ;
bbb b6 : /project-a2//target ;
bbb b7 : /project-a3//target ;

use-project id3 : a ;
""")
    t.write("a/source._a", "")
    t.write("a/jamfile.jam", """\
project project-a1 ;
project /project-a2 ;
import alias ;
alias target : source._a ;
project /project-a3 ;
""")

    t.run_build_system()
    t.expect_addition("bin/$toolset/b%d._b" % x for x in range(1, 8))
    t.expect_nothing_more()

    t.cleanup()


def test_repeated_ids_for_different_projects():
    t = BoostBuild.Tester()

    t.write("a/jamfile.jam", "")
    t.write("jamroot.jam", "project foo ; use-project foo : a ;")
    t.run_build_system(status=1)
    t.expect_output_lines("""\
error: Attempt to redeclare already registered project id '/foo'.
error: Original project:
error:     Name: Jamfile<*>
error:     Module: Jamfile<*>
error:     Main id: /foo
error:     File: jamroot.jam
error:     Location: .
error: New project:
error:     Module: Jamfile<*>
error:     File: a*jamfile.jam
error:     Location: a""")

    t.write("jamroot.jam", "use-project foo : a ; project foo ;")
    t.run_build_system(status=1)
    t.expect_output_lines("""\
error: Attempt to redeclare already registered project id '/foo'.
error: Original project:
error:     Name: Jamfile<*>
error:     Module: Jamfile<*>
error:     Main id: /foo
error:     File: jamroot.jam
error:     Location: .
error: New project:
error:     Module: Jamfile<*>
error:     File: a*jamfile.jam
error:     Location: a""")

    t.write("jamroot.jam", """\
import modules ;
import project ;
modules.call-in [ project.load a ] : project foo ;
project foo ;
""")
    t.run_build_system(status=1)
    t.expect_output_lines("""\
error: at jamroot.jam:4
error: Attempt to redeclare already registered project id '/foo'.
error: Original project:
error:     Name: Jamfile<*>
error:     Module: Jamfile<*>
error:     Main id: /foo
error:     File: a*jamfile.jam
error:     Location: a
error: New project:
error:     Module: Jamfile<*>
error:     File: jamroot.jam
error:     Location: .""")

    t.cleanup()


def test_repeated_ids_for_same_project():
    t = BoostBuild.Tester()

    t.write("jamroot.jam", "project foo ; project foo ;")
    t.run_build_system()

    t.write("jamroot.jam", "project foo ; use-project foo : . ;")
    t.run_build_system()

    t.write("jamroot.jam", "project foo ; use-project foo : ./. ;")
    t.run_build_system()

    t.write("jamroot.jam", """\
project foo ;
use-project foo : . ;
use-project foo : ./aaa/.. ;
use-project foo : ./. ;
""")
    t.run_build_system()

    # On Windows we have a case-insensitive file system and we can use
    # backslashes as path separators.
    # FIXME: Make a similar test pass on Cygwin.
    if sys.platform in ['win32']:
        t.write("a/fOo bAr/b/jamfile.jam", "")
        t.write("jamroot.jam", r"""
use-project bar : "a/foo bar/b" ;
use-project bar : "a/foO Bar/b" ;
use-project bar : "a/foo BAR/b/" ;
use-project bar : "a\\.\\FOO bar\\b\\" ;
""")
        t.run_build_system()
        t.rm("a")

    t.write("bar/jamfile.jam", "")
    t.write("jamroot.jam", """\
use-project bar : bar ;
use-project bar : bar/ ;
use-project bar : bar// ;
use-project bar : bar/// ;
use-project bar : bar//// ;
use-project bar : bar/. ;
use-project bar : bar/./ ;
use-project bar : bar/////./ ;
use-project bar : bar/../bar/xxx/.. ;
use-project bar : bar/..///bar/xxx///////.. ;
use-project bar : bar/./../bar/xxx/.. ;
use-project bar : bar/.////../bar/xxx/.. ;
use-project bar : bar/././../bar/xxx/.. ;
use-project bar : bar/././//////////../bar/xxx/.. ;
use-project bar : bar/.///.////../bar/xxx/.. ;
use-project bar : bar/./././xxx/.. ;
use-project bar : bar/xxx////.. ;
use-project bar : bar/xxx/.. ;
use-project bar : bar///////xxx/.. ;
""")
    t.run_build_system()
    t.rm("bar")

    # On Windows we have a case-insensitive file system and we can use
    # backslashes as path separators.
    # FIXME: Make a similar test pass on Cygwin.
    if sys.platform in ['win32']:
        t.write("baR/jamfile.jam", "")
        t.write("jamroot.jam", r"""
use-project bar : bar ;
use-project bar : BAR ;
use-project bar : bAr ;
use-project bar : bAr/ ;
use-project bar : bAr\\ ;
use-project bar : bAr\\\\ ;
use-project bar : bAr\\\\///// ;
use-project bar : bAr/. ;
use-project bar : bAr/./././ ;
use-project bar : bAr\\.\\.\\.\\ ;
use-project bar : bAr\\./\\/.\\.\\ ;
use-project bar : bAr/.\\././ ;
use-project bar : Bar ;
use-project bar : BaR ;
use-project bar : BaR/./../bAr/xxx/.. ;
use-project bar : BaR/./..\\bAr\\xxx/.. ;
use-project bar : BaR/xxx/.. ;
use-project bar : BaR///\\\\\\//xxx/.. ;
use-project bar : Bar\\xxx/.. ;
use-project bar : BAR/xXx/.. ;
use-project bar : BAR/xXx\\\\/\\/\\//\\.. ;
""")
        t.run_build_system()
        t.rm("baR")

    t.cleanup()


def test_unresolved_project_references():
    t = BoostBuild.Tester()

    __write_appender(t, "appender.jam")
    t.write("a/source._a", "")
    t.write("a/jamfile.jam", "import alias ; alias target : source._a ;")
    t.write("jamroot.jam", """\
import type ;
type.register AAA : _a ;
type.register BBB : _b ;

import appender ;
appender.register aaa-to-bbb : AAA : BBB ;

use-project foo : a ;

bbb b1 : a//target ;
bbb b2 : /foo//target ;
bbb b-invalid : invalid//target ;
bbb b-root-invalid : /invalid//target ;
bbb b-missing-root : foo//target ;
bbb b-invalid-target : /foo//invalid ;
""")

    t.run_build_system(["b1", "b2"])
    t.expect_addition("bin/$toolset/debug/b%d._b" % x for x in range(1, 3))
    t.expect_nothing_more()

    t.run_build_system(["b-invalid"], status=1)
    t.expect_output_lines("""\
error: Unable to find file or target named
error:     'invalid//target'
error: referred to from project at
error:     '.'
error: could not resolve project reference 'invalid'""")

    t.run_build_system(["b-root-invalid"], status=1)
    t.expect_output_lines("""\
error: Unable to find file or target named
error:     '/invalid//target'
error: referred to from project at
error:     '.'
error: could not resolve project reference '/invalid'""")

    t.run_build_system(["b-missing-root"], status=1)
    t.expect_output_lines("""\
error: Unable to find file or target named
error:     'foo//target'
error: referred to from project at
error:     '.'
error: could not resolve project reference 'foo' - possibly missing a """
    "leading slash ('/') character.")

    t.run_build_system(["b-invalid-target"], status=1)
    t.expect_output_lines("""\
error: Unable to find file or target named
error:     '/foo//invalid'
error: referred to from project at
error:     '.'""")
    t.expect_output_lines("*could not resolve project reference*", False)

    t.cleanup()


def __write_appender(t, name):
    t.write(name,
r"""# Copyright 2012 Jurko Gospodnetic
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

#   Support for registering test generators that construct their targets by
# simply appending their given input data, e.g. list of sources & targets.

import "class" : new ;
import generators ;
import modules ;
import sequence ;

rule register ( id composing ? : source-types + : target-types + )
{
    local caller-module = [ CALLER_MODULE ] ;
    id = $(caller-module).$(id) ;
    local g = [ new generator $(id) $(composing) : $(source-types) :
        $(target-types) ] ;
    $(g).set-rule-name $(__name__).appender ;
    generators.register $(g) ;
    return $(id) ;
}

if [ modules.peek : NT ]
{
    X = ")" ;
    ECHO_CMD = (echo. ;
}
else
{
    X = \" ;
    ECHO_CMD = "echo $(X)" ;
}

local appender-runs ;

# We set up separate actions for building each target in order to avoid having
# to iterate over them in action (i.e. shell) code. We have to be extra careful
# though to achieve the exact same effect as if doing all the work in just one
# action. Otherwise Boost Jam might, under some circumstances, run only some of
# our actions. To achieve this we register a series of actions for all the
# targets (since they all have the same target list - either all or none of them
# get run independent of which target actually needs to get built), each
# building only a single target. Since all our actions use the same targets, we
# can not use 'on-target' parameters to pass data to a specific action so we
# pass them using the second 'sources' parameter which our actions then know how
# to interpret correctly. This works well since Boost Jam does not automatically
# add dependency relations between specified action targets & sources and so the
# second argument, even though most often used to pass in a list of sources, can
# actually be used for passing in any type of information.
rule appender ( targets + : sources + : properties * )
{
    appender-runs = [ CALC $(appender-runs:E=0) + 1 ] ;
    local target-index = 0 ;
    local target-count = [ sequence.length $(targets) ] ;
    local original-targets ;
    for t in $(targets)
    {
        target-index = [ CALC $(target-index) + 1 ] ;
        local appender-run = $(appender-runs) ;
        if $(targets[2])-defined
        {
            appender-run += [$(target-index)/$(target-count)] ;
        }
        append $(targets) : $(appender-run:J=" ") $(t) $(sources) ;
    }
}

actions append
{
    $(ECHO_CMD)-------------------------------------------------$(X)
    $(ECHO_CMD)Appender run: $(>[1])$(X)
    $(ECHO_CMD)Appender run: $(>[1])$(X)>> "$(>[2])"
    $(ECHO_CMD)Target group: $(<:J=' ')$(X)
    $(ECHO_CMD)Target group: $(<:J=' ')$(X)>> "$(>[2])"
    $(ECHO_CMD)      Target: '$(>[2])'$(X)
    $(ECHO_CMD)      Target: '$(>[2])'$(X)>> "$(>[2])"
    $(ECHO_CMD)     Sources: '$(>[3-]:J=' ')'$(X)
    $(ECHO_CMD)     Sources: '$(>[3-]:J=' ')'$(X)>> "$(>[2])"
    $(ECHO_CMD)=================================================$(X)
    $(ECHO_CMD)-------------------------------------------------$(X)>> "$(>[2])"
}
""")


test_assigning_project_ids()
test_using_project_ids_in_target_references()
test_repeated_ids_for_same_project()
test_repeated_ids_for_different_projects()
test_unresolved_project_references()

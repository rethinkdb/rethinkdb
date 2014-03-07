#!/usr/bin/python

# Copyright 2002, 2003 Dave Abrahams
# Copyright 2002, 2003, 2004, 2005 Vladimir Prus
# Copyright 2012 Jurko Gospodnetic
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild
import re


def test_basic():
    t = BoostBuild.Tester(pass_d0=False)
    __write_appender(t, "appender.jam")
    t.write("a.cpp", "")
    t.write("b.cxx", "")
    t.write("c.tui", "")
    t.write("d.wd", "")
    t.write("e.cpp", "")
    t.write("x.l", "")
    t.write("y.x_pro", "")
    t.write("z.cpp", "")
    t.write("lib/c.cpp", "int bar() { return 0; }\n")
    t.write("lib/jamfile.jam", "my-lib auxilliary : c.cpp ;")
    t.write("jamroot.jam",
r"""import appender ;

import "class" : new ;
import generators ;
import type ;


################################################################################
#
#   We use our own custom EXE, LIB & OBJ target generators as using the regular
# ones would force us to have to deal with different compiler/linker specific
# 'features' that really have nothing to do with this test. For example, IBM XL
# C/C++ for AIX, V12.1 (Version: 12.01.0000.0000) compiler exits with a non-zero
# exit code and thus fails our build when run with a source file using an
# unknown suffix like '.marked_cpp'.
#
################################################################################

type.register MY_EXE : my_exe ;
type.register MY_LIB : my_lib ;
type.register MY_OBJ : my_obj ;

appender.register compile-c : C : MY_OBJ ;
appender.register compile-cpp : CPP : MY_OBJ ;
appender.register link-lib composing : MY_OBJ : MY_LIB ;
appender.register link-exe composing : MY_OBJ MY_LIB : MY_EXE ;


################################################################################
#
# LEX --> C
#
################################################################################

type.register LEX : l ;

appender.register lex-to-c : LEX : C ;


################################################################################
#
#        /--> tUI_H --\
# tUI --<              >--> CPP
#        \------------/
#
################################################################################

type.register tUI : tui ;
type.register tUI_H : tui_h ;

appender.register ui-to-cpp : tUI tUI_H : CPP ;
appender.register ui-to-h : tUI : tUI_H ;


################################################################################
#
#          /--> X1 --\
# X_PRO --<           >--> CPP
#          \--> X2 --/
#
################################################################################

type.register X1 : x1 ;
type.register X2 : x2 ;
type.register X_PRO : x_pro ;

appender.register x1-x2-to-cpp : X1 X2 : CPP ;
appender.register x-pro-to-x1-x2 : X_PRO : X1 X2 ;


################################################################################
#
#   When the main target type is NM_EXE, build OBJ from CPP-MARKED and not from
# anything else, e.g. directly from CPP.
#
################################################################################

type.register CPP_MARKED : marked_cpp : CPP ;
type.register POSITIONS : positions ;
type.register NM.TARGET.CPP : target_cpp : CPP ;
type.register NM_EXE : : MY_EXE ;

appender.register marked-to-target-cpp : CPP_MARKED : NM.TARGET.CPP ;
appender.register cpp-to-marked-positions : CPP : CPP_MARKED POSITIONS ;

class nm::target::cpp-obj-generator : generator
{
    rule __init__ ( id )
    {
        generator.__init__ $(id) : NM.TARGET.CPP : MY_OBJ ;
        generator.set-rule-name appender.appender ;
    }

    rule requirements ( )
    {
        return <main-target-type>NM_EXE ;
    }

    rule run ( project name ? : properties * : source : multiple ? )
    {
        if [ $(source).type ] = CPP
        {
            local converted = [ generators.construct $(project) : NM.TARGET.CPP
                : $(properties) : $(source) ] ;
            if $(converted)
            {
                return [ generators.construct $(project) : MY_OBJ :
                    $(properties) : $(converted[2]) ] ;
            }
        }
    }
}
generators.register [ new nm::target::cpp-obj-generator target-obj ] ;
generators.override target-obj : all ;


################################################################################
#
# A more complex test case scenario with the following generators:
#  1. WHL --> CPP, WHL_LR0, H, H(%_symbols)
#  2. DLP --> CPP
#  3. WD --> WHL(%_parser) DLP(%_lexer)
#  4. A custom generator of higher priority than generators 1. & 2. that helps
#     disambiguate between them when generating CPP files from WHL and DLP
#     sources.
#
################################################################################

type.register WHL : whl ;
type.register DLP : dlp ;
type.register WHL_LR0 : lr0 ;
type.register WD : wd ;

local whale-generator-id = [ appender.register whale : WHL : CPP WHL_LR0 H
    H(%_symbols) ] ;
local dolphin-generator-id = [ appender.register dolphin : DLP : CPP ] ;
appender.register wd : WD : WHL(%_parser) DLP(%_lexer) ;

class wd-to-cpp : generator
{
    rule __init__ ( id : sources * : targets * )
    {
        generator.__init__ $(id) : $(sources) : $(targets) ;
    }

    rule run ( project name ? : property-set : source )
    {
        local new-sources = $(source) ;
        if ! [ $(source).type ] in WHL DLP
        {
            local r1 = [ generators.construct $(project) $(name) : WHL :
                $(property-set) : $(source) ] ;
            local r2 = [ generators.construct $(project) $(name) : DLP :
                $(property-set) : $(source) ] ;
            new-sources = [ sequence.unique $(r1[2-]) $(r2[2-]) ] ;
        }

        local result ;
        for local i in $(new-sources)
        {
            local t = [ generators.construct $(project) $(name) : CPP :
                $(property-set) : $(i) ] ;
            result += $(t[2-]) ;
        }
        return $(result) ;
    }
}
generators.override $(__name__).wd-to-cpp : $(whale-generator-id) ;
generators.override $(__name__).wd-to-cpp : $(dolphin-generator-id) ;
generators.register [ new wd-to-cpp $(__name__).wd-to-cpp : : CPP ] ;


################################################################################
#
# Declare build targets.
#
################################################################################

# This should not cause two CPP --> MY_OBJ constructions for a.cpp or b.cpp.
my-exe a : a.cpp b.cxx obj_1 obj_2 c.tui d.wd x.l y.x_pro lib//auxilliary ;
my-exe f : a.cpp b.cxx obj_1 obj_2 lib//auxilliary ;

# This should cause two CPP --> MY_OBJ constructions for z.cpp.
my-obj obj_1 : z.cpp ;
my-obj obj_2 : z.cpp ;

nm-exe e : e.cpp ;
""")

    t.run_build_system()
    t.expect_addition("bin/$toolset/debug/" * BoostBuild.List("a.my_exe "
        "a.my_obj b.my_obj c.tui_h c.cpp c.my_obj d_parser.whl d_lexer.dlp "
        "d_parser.cpp d_lexer.cpp d_lexer.my_obj d_parser.lr0 d_parser.h "
        "d_parser.my_obj d_parser_symbols.h x.c x.my_obj y.x1 y.x2 y.cpp "
        "y.my_obj e.marked_cpp e.positions e.target_cpp e.my_obj e.my_exe "
        "f.my_exe obj_1.my_obj obj_2.my_obj"))
    t.expect_addition("lib/bin/$toolset/debug/" * BoostBuild.List("c.my_obj "
        "auxilliary.my_lib"))
    t.expect_nothing_more()

    folder = "bin/$toolset/debug"
    t.expect_content_lines("%s/obj_1.my_obj" % folder, "     Sources: 'z.cpp'")
    t.expect_content_lines("%s/obj_2.my_obj" % folder, "     Sources: 'z.cpp'")
    t.expect_content_lines("%s/a.my_obj" % folder, "     Sources: 'a.cpp'")

    lines = t.stdout().splitlines()
    source_lines = [x for x in lines if re.match("^     Sources: '", x)]
    if not __match_count_is(source_lines, "'z.cpp'", 2):
        BoostBuild.annotation("failure", "z.cpp must be compiled exactly "
            "twice.")
        t.fail_test(1)
    if not __match_count_is(source_lines, "'a.cpp'", 1):
        BoostBuild.annotation("failure", "a.cpp must be compiled exactly "
            "once.")
        t.fail_test(1)
    t.cleanup()


def test_generated_target_names():
    """
      Test generator generated target names. Unless given explicitly, target
    names should be determined based on their specified source names. All
    sources for generating a target need to have matching names in order for
    Boost Build to be able to implicitly determine the target's name.

      We use the following target generation structure with differently named
    BBX targets:
                       /---> BB1 ---\
                AAA --<----> BB2 ---->--> CCC --(composing)--> DDD
                       \---> BB3 ---/

      The extra generator at the end is needed because generating a top-level
    CCC target directly would requires us to explicitly specify a name for it.
    The extra generator needs to be composing in order not to explicitly
    request a specific name for its CCC source target based on its own target
    name.

      We also check for a regression where only the first two sources were
    checked to see if their names match. Note that we need to try out all file
    renaming combinations as we do not know what ordering Boost Build is going
    to use when passing in those files as generator sources.

    """
    jamfile_template = """\
import type ;
type.register AAA : _a ;
type.register BB1 : _b1 ;
type.register BB2 : _b2 ;
type.register BB3 : _b3 ;
type.register CCC : _c ;
type.register DDD : _d ;

import appender ;
appender.register aaa-to-bbX           : AAA         : BB1%s BB2%s BB3%s ;
appender.register bbX-to-ccc           : BB1 BB2 BB3 : CCC ;
appender.register ccc-to-ddd composing : CCC         : DDD ;

ddd _xxx : _xxx._a ;
"""

    t = BoostBuild.Tester(pass_d0=False)
    __write_appender(t, "appender.jam")
    t.write("_xxx._a", "")

    def test_one(t, rename1, rename2, rename3, status):
        def f(rename):
            if rename: return "(%_x)"
            return ""

        jamfile = jamfile_template % (f(rename1), f(rename2), f(rename3))
        t.write("jamroot.jam", jamfile, wait=False)

        #   Remove any preexisting targets left over from a previous test run
        # so we do not have to be careful about tracking which files have been
        # newly added and which preexisting ones have only been modified.
        t.rm("bin")

        t.run_build_system(status=status)

        if status:
            t.expect_output_lines("*.bbX-to-ccc: source targets have "
                "different names: cannot determine target name")
        else:
            def suffix(rename):
                if rename: return "_x"
                return ""
            name = "bin/$toolset/debug/_xxx"
            e = t.expect_addition
            e("%s%s._b1" % (name, suffix(rename1)))
            e("%s%s._b2" % (name, suffix(rename2)))
            e("%s%s._b3" % (name, suffix(rename3)))
            e("%s%s._c" % (name, suffix(rename1 and rename2 and rename3)))
            e("%s._d" % name)
        t.expect_nothing_more()

    test_one(t, False, False, False, status=0)
    test_one(t, True , False, False, status=1)
    test_one(t, False, True , False, status=1)
    test_one(t, False, False, True , status=1)
    test_one(t, True , True , False, status=1)
    test_one(t, True , False, True , status=1)
    test_one(t, False, True , True , status=1)
    test_one(t, True , True , True , status=0)
    t.cleanup()


def __match_count_is(lines, pattern, expected):
    count = 0
    for x in lines:
        if re.search(pattern, x):
            count += 1
        if count > expected:
            return False
    return count == expected


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


test_basic()
test_generated_target_names()

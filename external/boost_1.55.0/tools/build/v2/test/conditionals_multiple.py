#!/usr/bin/python

# Copyright 2008 Jurko Gospodnetic
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Tests that properties conditioned on more than one other property work as
# expected.

import BoostBuild


###############################################################################
#
# test_multiple_conditions()
# --------------------------
#
###############################################################################

def test_multiple_conditions():
    """Basic tests for properties conditioned on multiple other properties."""

    t = BoostBuild.Tester(["--user-config=", "--ignore-site-config",
        "toolset=testToolset"], pass_toolset=False, use_test_config=False)

    t.write("testToolset.jam", """\
import feature ;
feature.extend toolset : testToolset ;
rule init ( ) { }
""")

    t.write("testToolset.py", """\
from b2.build import feature
feature.extend('toolset', ["testToolset"])
def init ( ): pass
""")

    t.write("jamroot.jam", """\
import feature ;
import notfile ;
import toolset ;

feature.feature description : : free incidental ;
feature.feature aaa : 1 0 : incidental ;
feature.feature bbb : 1 0 : incidental ;
feature.feature ccc : 1 0 : incidental ;

rule buildRule ( name : targets ? : properties * )
{
    for local description in [ feature.get-values description : $(properties) ]
    {
        ECHO "description:" /$(description)/ ;
    }
}

notfile testTarget1 : @buildRule : :
    <description>d
    <aaa>0:<description>a0
    <aaa>1:<description>a1
    <aaa>0,<bbb>0:<description>a0-b0
    <aaa>0,<bbb>1:<description>a0-b1
    <aaa>1,<bbb>0:<description>a1-b0
    <aaa>1,<bbb>1:<description>a1-b1
    <aaa>0,<bbb>0,<ccc>0:<description>a0-b0-c0
    <aaa>0,<bbb>0,<ccc>1:<description>a0-b0-c1
    <aaa>0,<bbb>1,<ccc>1:<description>a0-b1-c1
    <aaa>1,<bbb>0,<ccc>1:<description>a1-b0-c1
    <aaa>1,<bbb>1,<ccc>0:<description>a1-b1-c0
    <aaa>1,<bbb>1,<ccc>1:<description>a1-b1-c1 ;
""")

    t.run_build_system(["aaa=1", "bbb=1", "ccc=1"])
    t.expect_output_lines("description: /d/"              )
    t.expect_output_lines("description: /a0/"      , False)
    t.expect_output_lines("description: /a1/"             )
    t.expect_output_lines("description: /a0-b0/"   , False)
    t.expect_output_lines("description: /a0-b1/"   , False)
    t.expect_output_lines("description: /a1-b0/"   , False)
    t.expect_output_lines("description: /a1-b1/"          )
    t.expect_output_lines("description: /a0-b0-c0/", False)
    t.expect_output_lines("description: /a0-b0-c1/", False)
    t.expect_output_lines("description: /a0-b1-c1/", False)
    t.expect_output_lines("description: /a1-b0-c1/", False)
    t.expect_output_lines("description: /a1-b1-c0/", False)
    t.expect_output_lines("description: /a1-b1-c1/"       )

    t.run_build_system(["aaa=0", "bbb=0", "ccc=1"])
    t.expect_output_lines("description: /d/"              )
    t.expect_output_lines("description: /a0/"             )
    t.expect_output_lines("description: /a1/"      , False)
    t.expect_output_lines("description: /a0-b0/"          )
    t.expect_output_lines("description: /a0-b1/"   , False)
    t.expect_output_lines("description: /a1-b0/"   , False)
    t.expect_output_lines("description: /a1-b1/"   , False)
    t.expect_output_lines("description: /a0-b0-c0/", False)
    t.expect_output_lines("description: /a0-b0-c1/"       )
    t.expect_output_lines("description: /a0-b1-c1/", False)
    t.expect_output_lines("description: /a1-b0-c1/", False)
    t.expect_output_lines("description: /a1-b1-c0/", False)
    t.expect_output_lines("description: /a1-b1-c1/", False)

    t.run_build_system(["aaa=0", "bbb=0", "ccc=0"])
    t.expect_output_lines("description: /d/"              )
    t.expect_output_lines("description: /a0/"             )
    t.expect_output_lines("description: /a1/"      , False)
    t.expect_output_lines("description: /a0-b0/"          )
    t.expect_output_lines("description: /a0-b1/"   , False)
    t.expect_output_lines("description: /a1-b0/"   , False)
    t.expect_output_lines("description: /a1-b1/"   , False)
    t.expect_output_lines("description: /a0-b0-c0/"       )
    t.expect_output_lines("description: /a0-b0-c1/", False)
    t.expect_output_lines("description: /a0-b1-c1/", False)
    t.expect_output_lines("description: /a1-b0-c1/", False)
    t.expect_output_lines("description: /a1-b1-c0/", False)
    t.expect_output_lines("description: /a1-b1-c1/", False)

    t.cleanup()


###############################################################################
#
# test_multiple_conditions_with_toolset_version()
# -----------------------------------------------
#
###############################################################################

def test_multiple_conditions_with_toolset_version():
    """
      Regression tests for properties conditioned on the toolset version
    subfeature and some additional properties.

    """
    toolset = "testToolset" ;

    t = BoostBuild.Tester(["--user-config=", "--ignore-site-config"],
        pass_toolset=False, use_test_config=False)

    t.write(toolset + ".jam", """\
import feature ;
feature.extend toolset : %(toolset)s ;
feature.subfeature toolset %(toolset)s : version : 0 1 ;
rule init ( version ? ) { }
""" % {"toolset": toolset})

    t.write("testToolset.py", """\
from b2.build import feature
feature.extend('toolset', ["%(toolset)s"])
feature.subfeature('toolset', "%(toolset)s", "version", ['0','1'])
def init ( version ): pass
""" % {"toolset": toolset})

    t.write("jamroot.jam", """\
import feature ;
import notfile ;
import toolset ;

toolset.using testToolset ;

feature.feature description : : free incidental ;
feature.feature aaa : 0 1 : incidental ;
feature.feature bbb : 0 1 : incidental ;
feature.feature ccc : 0 1 : incidental ;

rule buildRule ( name : targets ? : properties * )
{
    local ttt = [ feature.get-values toolset                     : $(properties) ] ;
    local vvv = [ feature.get-values toolset-testToolset:version : $(properties) ] ;
    local aaa = [ feature.get-values aaa                         : $(properties) ] ;
    local bbb = [ feature.get-values bbb                         : $(properties) ] ;
    local ccc = [ feature.get-values ccc                         : $(properties) ] ;
    ECHO "toolset:" /$(ttt)/ "version:" /$(vvv)/ "aaa/bbb/ccc:" /$(aaa)/$(bbb)/$(ccc)/ ;
    for local description in [ feature.get-values description : $(properties) ]
    {
        ECHO "description:" /$(description)/ ;
    }
}

notfile testTarget1 : @buildRule : :
    <toolset>testToolset,<aaa>0:<description>t-a0
    <toolset>testToolset,<aaa>1:<description>t-a1

    <toolset>testToolset-0,<aaa>0:<description>t0-a0
    <toolset>testToolset-0,<aaa>1:<description>t0-a1
    <toolset>testToolset-1,<aaa>0:<description>t1-a0
    <toolset>testToolset-1,<aaa>1:<description>t1-a1

    <toolset>testToolset,<aaa>0,<bbb>0:<description>t-a0-b0
    <toolset>testToolset,<aaa>0,<bbb>1:<description>t-a0-b1
    <toolset>testToolset,<aaa>1,<bbb>0:<description>t-a1-b0
    <toolset>testToolset,<aaa>1,<bbb>1:<description>t-a1-b1

    <aaa>0,<toolset>testToolset,<bbb>0:<description>a0-t-b0
    <aaa>0,<toolset>testToolset,<bbb>1:<description>a0-t-b1
    <aaa>1,<toolset>testToolset,<bbb>0:<description>a1-t-b0
    <aaa>1,<toolset>testToolset,<bbb>1:<description>a1-t-b1

    <aaa>0,<bbb>0,<toolset>testToolset:<description>a0-b0-t
    <aaa>0,<bbb>1,<toolset>testToolset:<description>a0-b1-t
    <aaa>1,<bbb>0,<toolset>testToolset:<description>a1-b0-t
    <aaa>1,<bbb>1,<toolset>testToolset:<description>a1-b1-t

    <toolset>testToolset-0,<aaa>0,<bbb>0:<description>t0-a0-b0
    <toolset>testToolset-0,<aaa>0,<bbb>1:<description>t0-a0-b1
    <toolset>testToolset-0,<aaa>1,<bbb>0:<description>t0-a1-b0
    <toolset>testToolset-0,<aaa>1,<bbb>1:<description>t0-a1-b1
    <toolset>testToolset-1,<aaa>0,<bbb>0:<description>t1-a0-b0
    <toolset>testToolset-1,<aaa>0,<bbb>1:<description>t1-a0-b1
    <toolset>testToolset-1,<aaa>1,<bbb>0:<description>t1-a1-b0
    <toolset>testToolset-1,<aaa>1,<bbb>1:<description>t1-a1-b1

    <aaa>0,<toolset>testToolset-1,<bbb>0:<description>a0-t1-b0
    <aaa>0,<toolset>testToolset-1,<bbb>1:<description>a0-t1-b1
    <aaa>1,<toolset>testToolset-0,<bbb>0:<description>a1-t0-b0
    <aaa>1,<toolset>testToolset-0,<bbb>1:<description>a1-t0-b1

    <bbb>0,<aaa>1,<toolset>testToolset-0:<description>b0-a1-t0
    <bbb>0,<aaa>0,<toolset>testToolset-1:<description>b0-a0-t1
    <bbb>0,<aaa>1,<toolset>testToolset-1:<description>b0-a1-t1
    <bbb>1,<aaa>0,<toolset>testToolset-1:<description>b1-a0-t1
    <bbb>1,<aaa>1,<toolset>testToolset-0:<description>b1-a1-t0
    <bbb>1,<aaa>1,<toolset>testToolset-1:<description>b1-a1-t1 ;
""")

    t.run_build_system(["aaa=1", "bbb=1", "ccc=1", "toolset=%s-0" % toolset])
    t.expect_output_lines("description: /t-a0/"    , False)
    t.expect_output_lines("description: /t-a1/"           )
    t.expect_output_lines("description: /t0-a0/"   , False)
    t.expect_output_lines("description: /t0-a1/"          )
    t.expect_output_lines("description: /t1-a0/"   , False)
    t.expect_output_lines("description: /t1-a1/"   , False)
    t.expect_output_lines("description: /t-a0-b0/" , False)
    t.expect_output_lines("description: /t-a0-b1/" , False)
    t.expect_output_lines("description: /t-a1-b0/" , False)
    t.expect_output_lines("description: /t-a1-b1/"        )
    t.expect_output_lines("description: /a0-t-b0/" , False)
    t.expect_output_lines("description: /a0-t-b1/" , False)
    t.expect_output_lines("description: /a1-t-b0/" , False)
    t.expect_output_lines("description: /a1-t-b1/"        )
    t.expect_output_lines("description: /a0-b0-t/" , False)
    t.expect_output_lines("description: /a0-b1-t/" , False)
    t.expect_output_lines("description: /a1-b0-t/" , False)
    t.expect_output_lines("description: /a1-b1-t/"        )
    t.expect_output_lines("description: /t0-a0-b0/", False)
    t.expect_output_lines("description: /t0-a0-b1/", False)
    t.expect_output_lines("description: /t0-a1-b0/", False)
    t.expect_output_lines("description: /t0-a1-b1/"       )
    t.expect_output_lines("description: /t1-a0-b0/", False)
    t.expect_output_lines("description: /t1-a0-b1/", False)
    t.expect_output_lines("description: /t1-a1-b0/", False)
    t.expect_output_lines("description: /t1-a1-b1/", False)
    t.expect_output_lines("description: /a0-t1-b0/", False)
    t.expect_output_lines("description: /a0-t1-b1/", False)
    t.expect_output_lines("description: /a1-t0-b0/", False)
    t.expect_output_lines("description: /a1-t0-b1/"       )
    t.expect_output_lines("description: /b0-a1-t0/", False)
    t.expect_output_lines("description: /b0-a0-t1/", False)
    t.expect_output_lines("description: /b0-a1-t1/", False)
    t.expect_output_lines("description: /b1-a0-t1/", False)
    t.expect_output_lines("description: /b1-a1-t0/"       )
    t.expect_output_lines("description: /b1-a1-t1/", False)

    t.run_build_system(["aaa=1", "bbb=1", "ccc=1", "toolset=%s-1" % toolset])
    t.expect_output_lines("description: /t-a0/"    , False)
    t.expect_output_lines("description: /t-a1/"           )
    t.expect_output_lines("description: /t0-a0/"   , False)
    t.expect_output_lines("description: /t0-a1/"   , False)
    t.expect_output_lines("description: /t1-a0/"   , False)
    t.expect_output_lines("description: /t1-a1/"          )
    t.expect_output_lines("description: /t-a0-b0/" , False)
    t.expect_output_lines("description: /t-a0-b1/" , False)
    t.expect_output_lines("description: /t-a1-b0/" , False)
    t.expect_output_lines("description: /t-a1-b1/"        )
    t.expect_output_lines("description: /a0-t-b0/" , False)
    t.expect_output_lines("description: /a0-t-b1/" , False)
    t.expect_output_lines("description: /a1-t-b0/" , False)
    t.expect_output_lines("description: /a1-t-b1/"        )
    t.expect_output_lines("description: /a0-b0-t/" , False)
    t.expect_output_lines("description: /a0-b1-t/" , False)
    t.expect_output_lines("description: /a1-b0-t/" , False)
    t.expect_output_lines("description: /a1-b1-t/"        )
    t.expect_output_lines("description: /t0-a0-b0/", False)
    t.expect_output_lines("description: /t0-a0-b1/", False)
    t.expect_output_lines("description: /t0-a1-b0/", False)
    t.expect_output_lines("description: /t0-a1-b1/", False)
    t.expect_output_lines("description: /t1-a0-b0/", False)
    t.expect_output_lines("description: /t1-a0-b1/", False)
    t.expect_output_lines("description: /t1-a1-b0/", False)
    t.expect_output_lines("description: /t1-a1-b1/"       )
    t.expect_output_lines("description: /a0-t1-b0/", False)
    t.expect_output_lines("description: /a0-t1-b1/", False)
    t.expect_output_lines("description: /a1-t0-b0/", False)
    t.expect_output_lines("description: /a1-t0-b1/", False)
    t.expect_output_lines("description: /b0-a1-t0/", False)
    t.expect_output_lines("description: /b0-a0-t1/", False)
    t.expect_output_lines("description: /b0-a1-t1/", False)
    t.expect_output_lines("description: /b1-a0-t1/", False)
    t.expect_output_lines("description: /b1-a1-t0/", False)
    t.expect_output_lines("description: /b1-a1-t1/"       )

    t.cleanup()


###############################################################################
#
# main()
# ------
#
###############################################################################

test_multiple_conditions()
test_multiple_conditions_with_toolset_version()

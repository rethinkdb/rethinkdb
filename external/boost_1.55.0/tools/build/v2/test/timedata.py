#!/usr/bin/python

# Copyright 2005 David Abrahams
# Copyright 2008, 2012 Jurko Gospodnetic
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Tests the build step timing facilities.

# TODO: Missing tests:
# 1. 'time' target with a source target representing more than one virtual
#    target. This happens in practice, e.g. when using the time rule on a msvc
#    exe target whose generator actually constructs an EXE and a PDB target.
#    When this is done - only the main virtual target's constructing action
#    should be timed.
# 2. 'time' target with a source target representing a virtual target that
#    actually gets built by multiple actions run in sequence. In that case a
#    separate timing result should be reported for each of those actions. This
#    happens in practice, e.g. when using the time rule on a msvc exe target
#    which first gets created as a result of some link action and then its
#    manifest gets embedded inside it as a resource using a separate action
#    (assuming an appropriate property has been set for this target - see the
#    msvc module for details).

import BoostBuild
import re


###############################################################################
#
# basic_jam_action_test()
# -----------------------
#
###############################################################################

def basic_jam_action_test():
    """Tests basic Jam action timing support."""

    t = BoostBuild.Tester(pass_toolset=0)

    t.write("file.jam", """\
rule time
{
    DEPENDS $(<) : $(>) ;
    __TIMING_RULE__ on $(>) = record_time $(<) ;
    DEPENDS all : $(<) ;
}

actions time
{
    echo $(>) user: $(__USER_TIME__) system: $(__SYSTEM_TIME__)
    echo timed from $(>) >> $(<)
}

rule record_time ( target : source : start end user system )
{
    __USER_TIME__ on $(target) = $(user) ;
    __SYSTEM_TIME__ on $(target) = $(system) ;
}

rule make
{
    DEPENDS $(<) : $(>) ;
}

actions make
{
    echo made from $(>) >> $(<)
}

time foo : bar ;
make bar : baz ;
""")

    t.write("baz", "nothing")

    expected_output = """\
\.\.\.found 4 targets\.\.\.
\.\.\.updating 2 targets\.\.\.
make bar
time foo
bar +user: [0-9\.]+ +system: +[0-9\.]+ *
\.\.\.updated 2 targets\.\.\.$
"""

    t.run_build_system(["-ffile.jam", "-d+1"], stdout=expected_output,
        match=lambda actual, expected: re.search(expected, actual, re.DOTALL))
    t.expect_addition("foo")
    t.expect_addition("bar")
    t.expect_nothing_more()

    t.cleanup()


###############################################################################
#
# boost_build_testing_support_timing_rule():
# ------------------------------------------
#
###############################################################################

def boost_build_testing_support_timing_rule():
    """
      Tests the target build timing rule provided by the Boost Build testing
    support system.

    """
    t = BoostBuild.Tester(use_test_config=False)

    t.write("aaa.cpp", "int main() {}\n")

    t.write("jamroot.jam", """\
import testing ;
exe my-exe : aaa.cpp ;
time my-time : my-exe ;
""")

    t.run_build_system()
    t.expect_addition("bin/$toolset/debug/aaa.obj")
    t.expect_addition("bin/$toolset/debug/my-exe.exe")
    t.expect_addition("bin/$toolset/debug/my-time.time")

    t.expect_content_lines("bin/$toolset/debug/my-time.time",
        "user: *[0-9] seconds")
    t.expect_content_lines("bin/$toolset/debug/my-time.time",
        "system: *[0-9] seconds")

    t.cleanup()


###############################################################################
#
# boost_build_testing_support_timing_rule_with_spaces_in_names()
# --------------------------------------------------------------
#
###############################################################################

def boost_build_testing_support_timing_rule_with_spaces_in_names():
    """
      Tests the target build timing rule provided by the Boost Build testing
    support system when used with targets contining spaces in their names.

    """
    t = BoostBuild.Tester(use_test_config=False)

    t.write("aaa bbb.cpp", "int main() {}\n")

    t.write("jamroot.jam", """\
import testing ;
exe "my exe" : "aaa bbb.cpp" ;
time "my time" : "my exe" ;
""")

    t.run_build_system()
    t.expect_addition("bin/$toolset/debug/aaa bbb.obj")
    t.expect_addition("bin/$toolset/debug/my exe.exe")
    t.expect_addition("bin/$toolset/debug/my time.time")

    t.expect_content_lines("bin/$toolset/debug/my time.time", "user: *")
    t.expect_content_lines("bin/$toolset/debug/my time.time", "system: *")

    t.cleanup()


###############################################################################
#
# main()
# ------
#
###############################################################################

basic_jam_action_test()
boost_build_testing_support_timing_rule()
boost_build_testing_support_timing_rule_with_spaces_in_names()
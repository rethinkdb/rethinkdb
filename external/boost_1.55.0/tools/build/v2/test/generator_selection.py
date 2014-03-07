#!/usr/bin/python

# Copyright 2008, 2012 Jurko Gospodnetic
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Tests that generators get selected correctly.
#
# We do not use the internal C++-compiler CPP --> OBJ generator to avoid
# problems with specific compilers or their configurations, e.g. IBM's AIX test
# runner 'AIX Version 5.3 TL7 SP5 (5300-07-05-0831)' using the 'IBM XL C/C++
# for AIX, V12.1 (Version: 12.01.0000.0000)' reporting errors when run with a
# source file whose suffix is not '.cpp'.

import BoostBuild


###############################################################################
#
# test_generator_added_after_already_building_a_target_of_its_target_type()
# -------------------------------------------------------------------------
#
###############################################################################

def test_generator_added_after_already_building_a_target_of_its_target_type():
    """
      Regression test for a Boost Build bug causing it to not use a generator
    if it got added after already building a target of its target type.

    """
    t = BoostBuild.Tester()

    t.write("dummy.cpp", "void f() {}\n")

    t.write("jamroot.jam", """\
import common ;
import generators ;
import type ;
type.register MY_OBJ : my_obj ;
generators.register-standard common.copy : CPP : MY_OBJ ;

# Building this dummy target must not cause a later defined CPP target type
# generator not to be recognized as viable.
my-obj dummy : dummy.cpp ;
alias the-other-obj : Other//other-obj ;
""")

    t.write("Other/source.extension", "A dummy source file.")

    t.write("Other/mygen.jam", """\
import common ;
import generators ;
import type ;
type.register MY_TYPE : extension ;
generators.register-standard $(__name__).generate-a-cpp-file : MY_TYPE : CPP ;
rule generate-a-cpp-file { ECHO Generating a CPP file... ; }
CREATE-FILE = [ common.file-creation-command ] ;
actions generate-a-cpp-file { $(CREATE-FILE) "$(<)" }
""")

    t.write("Other/mygen.py", """\
import b2.build.generators as generators
import b2.build.type as type

from b2.manager import get_manager

import os

type.register('MY_TYPE', ['extension'])
generators.register_standard('mygen.generate-a-cpp-file', ['MY_TYPE'], ['CPP'])
if os.name == 'nt':
    action = 'echo void g() {} > "$(<)"'
else:
    action = 'echo "void g() {}" > "$(<)"'
def f(*args):
    print "Generating a CPP file..."

get_manager().engine().register_action("mygen.generate-a-cpp-file", action,
    function=f)
""")

    t.write("Other/jamfile.jam", """\
import mygen ;
my-obj other-obj : source.extension ;
""")

    t.run_build_system()
    t.expect_output_lines("Generating a CPP file...")
    t.expect_addition("bin/$toolset/debug/dummy.my_obj")
    t.expect_addition("Other/bin/$toolset/debug/other-obj.cpp")
    t.expect_addition("Other/bin/$toolset/debug/other-obj.my_obj")
    t.expect_nothing_more()

    t.cleanup()


###############################################################################
#
# test_using_a_derived_source_type_created_after_generator_already_used()
# -----------------------------------------------------------------------
#
###############################################################################

def test_using_a_derived_source_type_created_after_generator_already_used():
    """
      Regression test for a Boost Build bug causing it to not use a generator
    with a source type derived from one of the generator's sources but created
    only after already using the generateor.

    """
    t = BoostBuild.Tester()

    t.write("dummy.xxx", "Hello. My name is Peter Pan.\n")

    t.write("jamroot.jam", """\
import common ;
import generators ;
import type ;
type.register XXX : xxx ;
type.register YYY : yyy ;
generators.register-standard common.copy : XXX : YYY ;

# Building this dummy target must not cause a later defined XXX2 target type not
# to be recognized as a viable source type for building YYY targets.
yyy dummy : dummy.xxx ;
alias the-test-output : Other//other ;
""")

    t.write("Other/source.xxx2", "Hello. My name is Tinkerbell.\n")

    t.write("Other/jamfile.jam", """\
import type ;
type.register XXX2 : xxx2 : XXX ;
# We are careful not to do anything between defining our new XXX2 target type
# and using the XXX --> YYY generator that could potentially cover the Boost
# Build bug by clearing its internal viable source target type state.
yyy other : source.xxx2 ;
""")

    t.run_build_system()
    t.expect_addition("bin/$toolset/debug/dummy.yyy")
    t.expect_addition("Other/bin/$toolset/debug/other.yyy")
    t.expect_nothing_more()

    t.cleanup()


###############################################################################
#
# main()
# ------
#
###############################################################################

test_generator_added_after_already_building_a_target_of_its_target_type()
test_using_a_derived_source_type_created_after_generator_already_used()

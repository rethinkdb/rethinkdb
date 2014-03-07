#!/usr/bin/python

# Copyright 2008 Jurko Gospodnetic
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test that the expected default toolset is used when no toolset is explicitly
# specified on the command line or used from code via the using rule. Test that
# the default toolset is correctly used just like any other explicitly used
# toolset (e.g. toolset prerequisites, properties conditioned on toolset
# related features, etc.).
#
# Note that we need to ignore regular site/user/test configuration files to
# avoid them marking any toolsets not under our control as used.

import BoostBuild


# Line displayed by Boost Build when using the default toolset.
configuring_default_toolset_message = \
    'warning: Configuring default toolset "%s".'


###############################################################################
#
# test_conditions_on_default_toolset()
# ------------------------------------
#
###############################################################################

def test_conditions_on_default_toolset():
    """Test that toolset and toolset subfeature conditioned properties get
    applied correctly when the toolset is selected by default. Implicitly tests
    that we can use the set-default-toolset rule to set the default toolset to
    be used by Boost Build.
    """

    t = BoostBuild.Tester("--user-config= --ignore-site-config",
        pass_toolset=False, use_test_config=False)

    toolset_name           = "myCustomTestToolset"
    toolset_version        = "v"
    toolset_version_unused = "v_unused"
    message_loaded         = "Toolset '%s' loaded." % toolset_name
    message_initialized    = "Toolset '%s' initialized." % toolset_name ;

    # Custom toolset.
    t.write(toolset_name + ".jam", """
import feature ;
ECHO "%(message_loaded)s" ;
feature.extend toolset : %(toolset_name)s ;
feature.subfeature toolset %(toolset_name)s : version : %(toolset_version)s %(toolset_version_unused)s ;
rule init ( version ) { ECHO "%(message_initialized)s" ; }
""" % {'message_loaded'     : message_loaded     ,
    'message_initialized'   : message_initialized,
    'toolset_name'          : toolset_name       ,
    'toolset_version'       : toolset_version    ,
    'toolset_version_unused': toolset_version_unused})

    # Main Boost Build project script.
    t.write("jamroot.jam", """
import build-system ;
import errors ;
import feature ;
import notfile ;

build-system.set-default-toolset %(toolset_name)s : %(toolset_version)s ;

feature.feature description : : free incidental ;

# We use a rule instead of an action to avoid problems with action output not
# getting piped to stdout by the testing system.
rule buildRule ( names : targets ? : properties * )
{
    local descriptions = [ feature.get-values description : $(properties) ] ;
    ECHO "descriptions:" /$(descriptions)/ ;
    local toolset = [ feature.get-values toolset : $(properties) ] ;
    ECHO "toolset:" /$(toolset)/ ;
    local toolset-version = [ feature.get-values "toolset-$(toolset):version" : $(properties) ] ;
    ECHO "toolset-version:" /$(toolset-version)/ ;
}

notfile testTarget
    : @buildRule
    :
    :
    <description>stand-alone
    <toolset>%(toolset_name)s:<description>toolset
    <toolset>%(toolset_name)s-%(toolset_version)s:<description>toolset-version
    <toolset>%(toolset_name)s-%(toolset_version_unused)s:<description>toolset-version-unused ;
""" % {'toolset_name'       : toolset_name   ,
    'toolset_version'       : toolset_version,
    'toolset_version_unused': toolset_version_unused})

    t.run_build_system()
    t.expect_output_lines(configuring_default_toolset_message % toolset_name)
    t.expect_output_lines(message_loaded)
    t.expect_output_lines(message_initialized)
    t.expect_output_lines("descriptions: /stand-alone/ /toolset/ "
        "/toolset-version/")
    t.expect_output_lines("toolset: /%s/" % toolset_name)
    t.expect_output_lines("toolset-version: /%s/" % toolset_version)

    t.cleanup()


###############################################################################
#
# test_default_toolset_on_os()
# ----------------------------
#
###############################################################################

def test_default_toolset_on_os( os, expected_toolset ):
    """Test that the given toolset is used as the default toolset on the given
    os. Uses hardcoded knowledge of how Boost Build decides on which host OS it
    is currently running. Note that we must not do much after tricking Boost
    Build into believing it has a specific host OS as this might mess up other
    important internal Boost Build state.
    """

    t = BoostBuild.Tester("--user-config= --ignore-site-config",
        pass_toolset=False, use_test_config=False)

    t.write("jamroot.jam", "modules.poke os : .name : %s ;" % os)

    # We need to tell the test system to ignore stderr output as attempting to
    # load missing toolsets might cause random failures with which we are not
    # concerned in this test.
    t.run_build_system(stderr=None)
    t.expect_output_lines(configuring_default_toolset_message %
        expected_toolset)

    t.cleanup()


###############################################################################
#
# test_default_toolset_requirements()
# -----------------------------------
#
###############################################################################

def test_default_toolset_requirements():
    """Test that default toolset's requirements get applied correctly.
    """

    t = BoostBuild.Tester("--user-config= --ignore-site-config",
        pass_toolset=False, use_test_config=False,
        ignore_toolset_requirements=False)

    toolset_name = "customTestToolsetWithRequirements"

    # Custom toolset.
    t.write(toolset_name + ".jam", """
import feature ;
import toolset ;
feature.extend toolset : %(toolset_name)s ;
toolset.add-requirements <description>toolset-requirement ;
rule init ( ) { }
""" % {'toolset_name': toolset_name})

    # Main Boost Build project script.
    t.write("jamroot.jam", """
import build-system ;
import errors ;
import feature ;
import notfile ;

build-system.set-default-toolset %(toolset_name)s ;

feature.feature description : : free incidental ;

# We use a rule instead of an action to avoid problems with action output not
# getting piped to stdout by the testing system.
rule buildRule ( names : targets ? : properties * )
{
    local descriptions = [ feature.get-values description : $(properties) ] ;
    ECHO "descriptions:" /$(descriptions)/ ;
    local toolset = [ feature.get-values toolset : $(properties) ] ;
    ECHO "toolset:" /$(toolset)/ ;
}

notfile testTarget
    : @buildRule
    :
    :
    <description>target-requirement
    <description>toolset-requirement:<description>conditioned-requirement
    <description>unrelated-condition:<description>unrelated-description ;
""" % {'toolset_name': toolset_name})

    t.run_build_system()
    t.expect_output_lines(configuring_default_toolset_message % toolset_name)
    t.expect_output_lines("descriptions: /conditioned-requirement/ "
        "/target-requirement/ /toolset-requirement/")
    t.expect_output_lines("toolset: /%s/" % toolset_name)

    t.cleanup()


###############################################################################
#
# main()
# ------
#
###############################################################################

test_default_toolset_on_os("NT"         , "msvc")
test_default_toolset_on_os("LINUX"      , "gcc" )
test_default_toolset_on_os("CYGWIN"     , "gcc" )
test_default_toolset_on_os("SomeOtherOS", "gcc" )
test_default_toolset_requirements()
test_conditions_on_default_toolset()

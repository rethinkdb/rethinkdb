#!/usr/bin/python

# Copyright 2002 Dave Abrahams
# Copyright 2003, 2004 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild
import os.path
import re


def check_for_existing_boost_build_jam(t):
    """
      This test depends on no boost-build.jam file existing in any of the
    folders along the current folder's path. If it does exist, not only would
    this test fail but it could point to a completely wrong Boost Build
    installation, thus causing headaches when attempting to diagnose the
    problem. That is why we explicitly check for this scenario.

    """
    problem = find_up_to_root(t.workdir, "boost-build.jam")
    if problem:
        BoostBuild.annotation("misconfiguration", """\
This test expects to be run from a folder with no 'boost-build.jam' file in any
of the folders along its path.

Working folder:
  '%s'

Problematic boost-build.jam found at:
  '%s'

Please remove this file or change the test's working folder and rerun the test.
""" % (t.workdir, problem))
        t.fail_test(1, dump_stdio=False, dump_stack=False)


def find_up_to_root(folder, name):
    last = ""
    while last != folder:
        candidate = os.path.join(folder, name)
        if os.path.exists(candidate):
            return candidate
        last = folder
        folder = os.path.dirname(folder)


def match_re(actual, expected):
    return re.match(expected, actual, re.DOTALL) != None


t = BoostBuild.Tester(match=match_re, boost_build_path="", pass_toolset=0)
t.set_tree("startup")
check_for_existing_boost_build_jam(t)

t.run_build_system(status=1, stdout=
r"""Unable to load Boost\.Build: could not find "boost-build\.jam"
.*Attempted search from .* up to the root""")

t.run_build_system(status=1, subdir="no-bootstrap1",
    stdout=r"Unable to load Boost\.Build: could not find build system\."
    r".*attempted to load the build system by invoking"
    r".*'boost-build ;'"
    r'.*but we were unable to find "bootstrap\.jam"')

# Descend to a subdirectory which /does not/ contain a boost-build.jam file,
# and try again to test the crawl-up behavior.
t.run_build_system(status=1, subdir=os.path.join("no-bootstrap1", "subdir"),
    stdout=r"Unable to load Boost\.Build: could not find build system\."
    r".*attempted to load the build system by invoking"
    r".*'boost-build ;'"
    r'.*but we were unable to find "bootstrap\.jam"')

t.run_build_system(status=1, subdir="no-bootstrap2",
    stdout=r"Unable to load Boost\.Build: could not find build system\."
    r".*attempted to load the build system by invoking"
    r".*'boost-build \. ;'"
    r'.*but we were unable to find "bootstrap\.jam"')

t.run_build_system(status=1, subdir='no-bootstrap3', stdout=
r"""Unable to load Boost.Build
.*boost-build\.jam" was found.*
However, it failed to call the "boost-build" rule""")

# Test bootstrapping based on BOOST_BUILD_PATH.
t.run_build_system(["-sBOOST_BUILD_PATH=../boost-root/build"],
    subdir="bootstrap-env", stdout="build system bootstrapped")

# Test bootstrapping based on an explicit path in boost-build.jam.
t.run_build_system(subdir="bootstrap-explicit",
    stdout="build system bootstrapped")

t.cleanup()

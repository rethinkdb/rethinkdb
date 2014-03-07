#!/usr/bin/python

# Copyright (C) 2003. Pedro Ferreira
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild


###############################################################################
#
# test_folder_with_dot_in_name()
# ------------------------------
#
###############################################################################

def test_folder_with_dot_in_name(t):
    """
      Regression test: the 'tag' feature did not work in directories that had a
    dot in their name.

    """
    t.write("version-1.32.0/jamroot.jam", """\
project test : requirements <tag>@$(__name__).tag ;

rule tag ( name : type ? : property-set )
{
   # Do nothing, just make sure the rule is invoked OK.
   ECHO The tag rule has been invoked. ;
}
exe a : a.cpp ;
""")
    t.write("version-1.32.0/a.cpp", "int main() {}\n")

    t.run_build_system(subdir="version-1.32.0")
    t.expect_addition("version-1.32.0/bin/$toolset/debug/a.exe")
    t.expect_output_lines("The tag rule has been invoked.")


###############################################################################
#
# test_tag_property()
# -------------------
#
###############################################################################

def test_tag_property(t):
    """Basic tag property test."""

    t.write("jamroot.jam", """\
import virtual-target ;

rule tag ( name : type ? : property-set )
{
    local tags ;
    switch [ $(property-set).get <variant> ]
    {
        case debug   : tags += d ;
        case release : tags += r ;
    }
    switch [ $(property-set).get <link> ]
    {
        case shared : tags += s ;
        case static : tags += t ;
    }
    if $(tags)
    {
        return [ virtual-target.add-prefix-and-suffix $(name)_$(tags:J="")
            : $(type) : $(property-set) ] ;
    }
}

# Test both fully-qualified and local name of the rule
exe a : a.cpp : <tag>@$(__name__).tag ;
lib b : a.cpp : <tag>@tag ;
stage c : a ;
""")

    t.write("a.cpp", """\
int main() {}
#ifdef _MSC_VER
__declspec (dllexport) void x () {}
#endif
""")

    file_list = (
        BoostBuild.List("bin/$toolset/debug/a_ds.exe") +
        BoostBuild.List("bin/$toolset/debug/b_ds.dll") +
        BoostBuild.List("c/a_ds.exe") +
        BoostBuild.List("bin/$toolset/release/a_rs.exe") +
        BoostBuild.List("bin/$toolset/release/b_rs.dll") +
        BoostBuild.List("c/a_rs.exe") +
        BoostBuild.List("bin/$toolset/debug/link-static/a_dt.exe") +
        BoostBuild.List("bin/$toolset/debug/link-static/b_dt.lib") +
        BoostBuild.List("c/a_dt.exe") +
        BoostBuild.List("bin/$toolset/release/link-static/a_rt.exe") +
        BoostBuild.List("bin/$toolset/release/link-static/b_rt.lib") +
        BoostBuild.List("c/a_rt.exe"))

    variants = ["debug", "release", "link=static,shared"]

    t.run_build_system(variants)
    t.expect_addition(file_list)

    t.run_build_system(variants + ["clean"])
    t.expect_removal(file_list)


###############################################################################
#
# main()
# ------
#
###############################################################################

t = BoostBuild.Tester(use_test_config=False)

test_tag_property(t)
test_folder_with_dot_in_name(t)

t.cleanup()

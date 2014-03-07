#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild


###############################################################################
#
# test_alias_rule()
# -----------------
#
###############################################################################

def test_alias_rule(t):
    """Basic alias rule test."""

    t.write("jamroot.jam", """\
exe a : a.cpp ;
exe b : b.cpp ;
exe c : c.cpp ;

alias bin1 : a ;
alias bin2 : a b ;

alias src : s.cpp ;
exe hello : hello.cpp src ;
""")

    t.write("a.cpp", "int main() {}\n")
    t.copy("a.cpp", "b.cpp")
    t.copy("a.cpp", "c.cpp")
    t.copy("a.cpp", "hello.cpp")
    t.write("s.cpp", "")

    # Check that targets to which "bin1" refers are updated, and only those.
    t.run_build_system(["bin1"])
    t.expect_addition(BoostBuild.List("bin/$toolset/debug/") * "a.exe a.obj")
    t.expect_nothing_more()

    # Try again with "bin2"
    t.run_build_system(["bin2"])
    t.expect_addition(BoostBuild.List("bin/$toolset/debug/") * "b.exe b.obj")
    t.expect_nothing_more()

    # Try building everything, making sure 'hello' target is created.
    t.run_build_system()
    t.expect_addition(BoostBuild.List("bin/$toolset/debug/") * \
        "hello.exe hello.obj")
    t.expect_addition("bin/$toolset/debug/s.obj")
    t.expect_addition(BoostBuild.List("bin/$toolset/debug/") * "c.exe c.obj")
    t.expect_nothing_more()


###############################################################################
#
# test_alias_source_usage_requirements()
# --------------------------------------
#
###############################################################################

def test_alias_source_usage_requirements(t):
    """
      Check whether usage requirements are propagated via "alias". In case they
    are not, linking will fail as there will be no main() function defined
    anywhere in the source.
    
    """
    t.write("jamroot.jam", """\
lib l : l.cpp : : : <define>WANT_MAIN ;
alias la : l ;
exe main : main.cpp la ;
""")

    t.write("l.cpp", """\
void
#if defined(_WIN32)
__declspec(dllexport)
#endif
foo() {}
""")

    t.write("main.cpp", """\
#ifdef WANT_MAIN
int main() {}
#endif
""")

    t.run_build_system()


###############################################################################
#
# main()
# ------
#
###############################################################################

# We do not pass the '-d0' option to Boost Build here to get more detailed
# information in case of failure.
t = BoostBuild.Tester(pass_d0=False, use_test_config=False)

test_alias_rule(t)
test_alias_source_usage_requirements(t)

t.cleanup()

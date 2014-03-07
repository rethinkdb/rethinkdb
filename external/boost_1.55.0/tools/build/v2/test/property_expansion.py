#!/usr/bin/python

# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test that free property inside.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("jamroot.jam", """\
variant debug-AA : debug : <define>AA ;
alias all : hello ;
exe hello : hello.cpp ;
explicit hello ;
""")

t.write("hello.cpp", """\
#ifdef AA
int main() {}
#endif
""")

t.run_build_system(["debug-AA"])

t.cleanup()

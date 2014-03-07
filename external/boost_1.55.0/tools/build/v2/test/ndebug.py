#!/usr/bin/python

# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test that building with optimization brings NDEBUG define, and, more
# importantly, that dependency targets are built with NDEBUG as well, even if
# they are not directly requested.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("jamroot.jam", "exe hello : hello.cpp lib//lib1 ;")
t.write("hello.cpp", """\
#ifdef NDEBUG
void foo();
int main() { foo(); }
#endif
""")
t.write("lib/jamfile.jam", "lib lib1 : lib1.cpp ;")
t.write("lib/lib1.cpp", """\
#ifdef NDEBUG
void foo() {}
#endif
""")

# 'release' builds should get the NDEBUG define. We use static linking to avoid
# messing with imports/exports on Windows.
t.run_build_system(["link=static", "release"])

t.cleanup()

#!/usr/bin/python

# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Regression test: virtual targets with different dependency properties were
# considered different by 'virtual-target.register', but the code which
# determined the actual target paths ignored dependency properties so both
# targets ended up being in the same location.

import BoostBuild
import string


t = BoostBuild.Tester()

t.write("jamroot.jam", """\
lib foo : foo.cpp ;
exe hello : hello.cpp ;
exe hello2 : hello.cpp : <library>foo ;
""")

t.write("hello.cpp", "int main() {}\n")

t.write("foo.cpp", """\
#ifdef _WIN32
__declspec(dllexport)
#endif
void foo() {}
""")

t.run_build_system(["--no-error-backtrace"], status=1)
t.fail_test(string.find(t.stdout(), "Duplicate name of actual target") == -1)

t.cleanup()

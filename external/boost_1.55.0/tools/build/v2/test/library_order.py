#!/usr/bin/python

# Copyright 2004 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test that on compilers sensitive to library order on linker's command line,
# we generate the correct order.

import BoostBuild


t = BoostBuild.Tester(use_test_config=False)

t.write("main.cpp", """\
void a();
int main() { a(); }
""")

t.write("a.cpp", """\
void b();
void a() { b(); }
""")

t.write("b.cpp", """\
void c();
void b() { c(); }
""")

t.write("c.cpp", """\
void d();
void c() { d(); }
""")

t.write("d.cpp", """\
void d() {}
""")

# The order of libraries in 'main' is crafted so that we get an error unless we
# do something about the order ourselves.
t.write("jamroot.jam", """\
exe main : main.cpp libd libc libb liba ;
lib libd : d.cpp ;
lib libc : c.cpp : <link>static <use>libd ;
lib libb : b.cpp : <use>libc ;
lib liba : a.cpp : <use>libb ;
""")

t.run_build_system(["-d2"])
t.expect_addition("bin/$toolset/debug/main.exe")


# Test the order between searched libraries.
t.write("jamroot.jam", """\
exe main : main.cpp png z ;
lib png : z : <name>png ;
lib z : : <name>zzz ;
""")

t.run_build_system(["-a", "-n", "-d+2"])
t.fail_test(t.stdout().find("png") > t.stdout().find("zzz"))

t.write("jamroot.jam", """\
exe main : main.cpp png z ;
lib png : : <name>png ;
lib z : png : <name>zzz ;
""")

t.run_build_system(["-a", "-n", "-d+2"])
t.fail_test(t.stdout().find("png") < t.stdout().find("zzz"))


# Test the order between prebuilt libraries.
t.write("first.a", "")
t.write("second.a", "")
t.write("jamroot.jam", """\
exe main : main.cpp first second ;
lib first : second : <file>first.a ;
lib second : : <file>second.a ;
""")

t.run_build_system(["-a", "-n", "-d+2"])
t.fail_test(t.stdout().find("first") > t.stdout().find("second"))

t.write("jamroot.jam", """
exe main : main.cpp first second ;
lib first : : <file>first.a ;
lib second : first : <file>second.a ;
""")

t.run_build_system(["-a", "-n", "-d+2"])
t.fail_test(t.stdout().find("first") < t.stdout().find("second"))

t.cleanup()

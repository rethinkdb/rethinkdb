#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2002, 2003, 2004, 2006 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)


# Test that usage requirements on main targets work (and are propagated all the
# way up, and not only to direct dependants).
t.write("jamroot.jam", "")

# Note: 'lib cc ..', not 'lib c'. If using 'lib c: ...' the HP-CXX linker will
# confuse it with the system C runtime.
t.write("jamfile.jam", """\
lib b : b.cpp : <link>shared:<define>SHARED_B : :
    <define>FOO <link>shared:<define>SHARED_B ;
lib cc : c.cpp b ;
exe a : a.cpp cc ;
""")

t.write("b.cpp", """\
void
#if defined(_WIN32) && defined(SHARED_B)
__declspec(dllexport)
#endif
foo() {}
""")

t.write("c.cpp", """\
void
#if defined(_WIN32) && defined(SHARED_B)
__declspec(dllexport)
#endif
create_lib_please() {}
""")

t.write("a.cpp", """\
#ifdef FOO
void
# if defined(_WIN32) && defined(SHARED_B)
__declspec(dllexport)
# endif
foo() {}
#endif
int main() { foo(); }
""")

t.run_build_system()
t.run_build_system(["--clean"])


# Test that use requirements on main target work, when they are referred using
# 'dependency' features.

t.write("jamfile.jam", """\
lib b : b.cpp : <link>shared:<define>SHARED_B : : <define>FOO
    <link>shared:<define>SHARED_B ;
exe a : a.cpp : <use>b ;
""")

t.write("b.cpp", """\
void
#if defined(_WIN32) && defined(SHARED_B)
__declspec(dllexport)
#endif
foo() {}
""")

t.write("a.cpp", """\
#ifdef FOO
int main() {}
#endif
""")

t.run_build_system()
t.run_build_system(["--clean"])


# Test that usage requirements on a project work.
t.write("jamfile.jam", "exe a : a.cpp lib//b ;")

t.write("lib/jamfile.jam", """\
project
   : requirements <link>shared:<define>SHARED_B
   : usage-requirements <define>FOO <link>shared:<define>SHARED_B ;
lib b : b.cpp ;
""")

t.write("lib/b.cpp", """\
void
#if defined(_WIN32) && defined(SHARED_B)
__declspec(dllexport)
#endif
foo() {}
""")

t.run_build_system()


# Test that use requirements are inherited correctly.
t.write("jamfile.jam", "exe a : a.cpp lib/1//b ;")

t.write("a.cpp", """\
#if defined(FOO) && defined(ZOO)
void foo() {}
#endif
int main() { foo(); }
""")

t.write("lib/jamfile.jam", """\
project : requirements : usage-requirements <define>FOO ;
""")

t.write("lib/1/jamfile.jam", """\
project
   : requirements <link>shared:<define>SHARED_B
   : usage-requirements <define>ZOO <link>shared:<define>SHARED_B ;
lib b : b.cpp ;
""")

t.write("lib/1/b.cpp", """\
void
#if defined(_WIN32) && defined(SHARED_B)
__declspec(dllexport)
#endif
foo() {}
""")

t.run_build_system()
t.run_build_system(["--clean"])


# Test that we correctly handle dependency features in usage requirements on
# target.
t.write("jamfile.jam", """\
lib b : b.cpp : <link>shared:<define>SHARED_B : : <define>FOO
    <link>shared:<define>SHARED_B ;

# Here's the test: we should correctly handle dependency feature and get usage
# requirements from 'b'.
lib cc : c.cpp : <link>shared:<define>SHARED_C : : <library>b ;

# This will build only if <define>FOO was propagated from 'c'.
exe a : a.cpp cc ;
""")

t.write("a.cpp", """\
#ifdef FOO
void
# if defined(_WIN32) && defined(SHARED_B)
__declspec(dllexport)
# endif
foo();
#endif

int main() { foo(); }
""")

t.write("c.cpp", """\
int
#if defined(_WIN32) && defined(SHARED_C)
__declspec(dllexport)
#endif
must_export_something;
""")

t.run_build_system()
t.run_build_system(["--clean"])


# Test correct handling of dependency features in project requirements.
t.write("jamfile.jam", "exe a : a.cpp lib1//cc ;")

t.write("lib1/jamfile.jam", """\
project
    : requirements <link>shared:<define>SHARED_C
    : usage-requirements <library>../lib2//b <link>shared:<define>SHARED_C ;
lib cc : c.cpp ;
""")

t.write("lib1/c.cpp", """\
int
#if defined(_WIN32) && defined(SHARED_C)
__declspec(dllexport)
#endif
must_export_something;
""")

t.write("lib2/jamfile.jam", """\
lib b : b.cpp : <link>shared:<define>SHARED_B : : <define>FOO
    <link>shared:<define>SHARED_B ;
""")

t.copy("b.cpp", "lib2/b.cpp")

t.run_build_system()


# Test that targets listed in dependency features in usage requirements are
# built with the correct properties.
t.rm(".")

t.write("jamroot.jam", "")
t.write("jamfile.jam", """\
lib main : main.cpp : <use>libs//lib1 : : <library>libs//lib1 ;
exe hello : hello.cpp main : ;
""")

t.write("main.cpp", """\
void
#if defined(_WIN32) && defined(SHARED_LIB1)
__declspec(dllimport)
#endif
foo();

int main() { foo(); }
""")

t.write("hello.cpp", "\n")
t.write("libs/a.cpp", """\
void
#if defined(_WIN32) && defined(SHARED_LIB1)
__declspec(dllexport)
#endif
foo() {}
""")


# This library should be built with the same properties as 'main'. This is a
# regression test for a bug when they were generated with empty properties, and
# there were ambiguities between variants.
t.write("libs/jamfile.jam", """\
lib lib1 : a_d.cpp : <variant>debug <link>shared:<define>SHARED_LIB1 : :
    <link>shared:<define>SHARED_LIB1 ;
lib lib1 : a.cpp : <variant>release <link>shared:<define>SHARED_LIB1 : :
    <link>shared:<define>SHARED_LIB1 ;
""")

t.write("libs/a_d.cpp", """\
void
#if defined(_WIN32) && defined(SHARED_LIB1)
__declspec(dllexport)
#endif
foo() {}
""")

t.run_build_system(["link=static"])
t.expect_addition("libs/bin/$toolset/debug/link-static/a_d.obj")


# Test that indirect conditionals are respected in usage requirements.
t.rm(".")

t.write("jamroot.jam", """\
rule has-foo ( properties * ) { return <define>HAS_FOO ; }
exe a : a.cpp b ;
lib b : b.cpp : <link>static : : <conditional>@has-foo ;
""")

t.write("a.cpp", """\
#ifdef HAS_FOO
void foo();
int main() { foo(); }
#endif
""")

t.write("b.cpp", """\
void
#if defined(_WIN32) && defined(SHARED_B)
__declspec(dllexport)
#endif
foo() {}
""")

t.run_build_system()
t.expect_addition("bin/$toolset/debug/a.exe")

t.cleanup()

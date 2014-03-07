#!/usr/bin/python

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

# First check some startup.

t.write("jamroot.jam", "")
t.write("jamfile.jam", """\
exe a : a.cpp b ;
lib b : b.cpp ;
""")

t.write("a.cpp", """\
void
# ifdef _WIN32
__declspec(dllimport)
# endif
foo();
int main() { foo(); }
""")

t.write("b.cpp", """\
#ifdef MACROS
void
# ifdef _WIN32
__declspec(dllexport)
# endif
foo() {}
#endif

# ifdef _WIN32
int __declspec(dllexport) force_implib_creation;
# endif
""")

t.run_build_system(["define=MACROS"])
t.expect_addition("bin/$toolset/debug/"
                  * (BoostBuild.List("a.obj b.obj b.dll a.exe")))


# When building a debug version, the 'define' still applies.
t.rm("bin")
t.run_build_system(["debug", "define=MACROS"])
t.expect_addition("bin/$toolset/debug/"
                  * (BoostBuild.List("a.obj b.obj b.dll a.exe")))


# When building a release version, the 'define' still applies.
t.write("jamfile.jam", """\
exe a : a.cpp b : <variant>debug ;
lib b : b.cpp ;
""")
t.rm("bin")
t.run_build_system(["release", "define=MACROS"])


# Regression test: direct build request was not working when there was more
# than one level of 'build-project'.
t.rm(".")
t.write("jamroot.jam", "")
t.write("jamfile.jam", "build-project a ;")
t.write("a/jamfile.jam", "build-project b ;")
t.write("a/b/jamfile.jam", "")
t.run_build_system(["release"])

t.cleanup()

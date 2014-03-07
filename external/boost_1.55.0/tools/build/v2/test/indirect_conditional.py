#!/usr/bin/python

# Copyright (C) 2006. Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

def test_basic():
    t = BoostBuild.Tester(use_test_config=False)

    t.write("jamroot.jam", """\
exe a1 : a1.cpp : <conditional>@a1-rule ;
rule a1-rule ( properties * )
{
    if <variant>debug in $(properties)
    {
        return <define>OK ;
    }
}

exe a2 : a2.cpp : <conditional>@$(__name__).a2-rule
    <variant>debug:<optimization>speed ;
rule a2-rule ( properties * )
{
    if <optimization>speed in $(properties)
    {
        return <define>OK ;
    }
}

exe a3 : a3.cpp :
    <conditional>@$(__name__).a3-rule-1
    <conditional>@$(__name__).a3-rule-2 ;
rule a3-rule-1 ( properties * )
{
    if <optimization>speed in $(properties)
    {
        return <define>OK ;
    }
}
rule a3-rule-2 ( properties * )
{
    if <variant>debug in $(properties)
    {
        return <optimization>speed ;
    }
}
""")

    t.write("a1.cpp", "#ifdef OK\nint main() {}\n#endif\n")
    t.write("a2.cpp", "#ifdef OK\nint main() {}\n#endif\n")
    t.write("a3.cpp", "#ifdef OK\nint main() {}\n#endif\n")

    t.run_build_system()

    t.expect_addition("bin/$toolset/debug/a1.exe")
    t.expect_addition("bin/$toolset/debug/optimization-speed/a2.exe")
    t.expect_addition("bin/$toolset/debug/optimization-speed/a3.exe")

    t.cleanup()


def test_glob_in_indirect_conditional():
    """
      Regression test: project-rules.glob rule run from inside an indirect
    conditional should report an error as it depends on the 'currently loaded
    project' concept and indirect conditional rules get called only after all
    the project modules have already finished loading.

    """
    t = BoostBuild.Tester(use_test_config=False)

    t.write("jamroot.jam", """\
use-project /library-example/foo : util/foo ;
build-project app ;
""")
    t.write("app/app.cpp", "int main() {}\n");
    t.write("app/jamfile.jam", "exe app : app.cpp /library-example/foo//bar ;")
    t.write("util/foo/bar.cpp", """\
#ifdef _WIN32
__declspec(dllexport)
#endif
void foo() {}
""")
    t.write("util/foo/jamfile.jam", """\
rule print-my-sources ( properties * )
{
    ECHO My sources: ;
    ECHO [ glob *.cpp ] ;
}
lib bar : bar.cpp : <conditional>@print-my-sources ;
""")

    t.run_build_system(status=1)
    t.expect_output_lines(["My sources:", "bar.cpp"], False)
    t.expect_output_lines("error: Reference to the project currently being "
        "loaded requested when there was no project module being loaded.")

    t.cleanup()


test_basic()
test_glob_in_indirect_conditional()

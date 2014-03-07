#!/usr/bin/python

# Copyright (C) 2003. Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Test that the <dll-path> property is correctly set when using
# <hardcode-dll-paths>true.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

# The point of this test is to have exe "main" which uses library "b", which
# uses library "a". When "main" is built with <hardcode-dll-paths>true, paths
# to both libraries should be present as values of <dll-path> feature. We
# create a special target type which reports <dll-path> values on its sources
# and compare the list of found values with out expectations.

t.write("jamroot.jam", "using dll_paths ;")
t.write("jamfile.jam", """\
exe main : main.cpp b//b ;
explicit main ;
path-list mp : main ;
""")

t.write("main.cpp", "int main() {}\n")
t.write("dll_paths.jam", """\
import "class" : new ;
import feature ;
import generators ;
import print ;
import sequence ;
import type ;

rule init ( )
{
    type.register PATH_LIST : pathlist ;

    class dll-paths-list-generator : generator
    {
        rule __init__ ( )
        {
            generator.__init__ dll_paths.list : EXE : PATH_LIST ;
        }

        rule generated-targets ( sources + : property-set : project name ? )
        {
            local dll-paths ;
            for local s in $(sources)
            {
                local a = [ $(s).action ] ;
                if $(a)
                {
                    local p = [ $(a).properties ] ;
                    dll-paths += [ $(p).get <dll-path> ] ;
                }
            }
            return [ generator.generated-targets $(sources) :
                [ $(property-set).add-raw $(dll-paths:G=<dll-path>) ] :
                $(project) $(name) ] ;
        }
    }
    generators.register [ new dll-paths-list-generator ] ;
}

rule list ( target : sources * : properties * )
{
    local paths = [ feature.get-values <dll-path> : $(properties) ] ;
    paths = [ sequence.insertion-sort $(paths) ] ;
    print.output $(target) ;
    print.text $(paths) ;
}
""")

t.write("dll_paths.py", """\
import bjam

import b2.build.type as type
import b2.build.generators as generators

from b2.manager import get_manager

def init():
    type.register("PATH_LIST", ["pathlist"])

    class DllPathsListGenerator(generators.Generator):

        def __init__(self):
            generators.Generator.__init__(self, "dll_paths.list", False,
                ["EXE"], ["PATH_LIST"])

        def generated_targets(self, sources, ps, project, name):
            dll_paths = []
            for s in sources:
                a = s.action()
                if a:
                    p = a.properties()
                    dll_paths += p.get('dll-path')
            dll_paths.sort()
            return generators.Generator.generated_targets(self, sources,
                ps.add_raw(["<dll-path>" + p for p in dll_paths]), project,
                name)

    generators.register(DllPathsListGenerator())

command = \"\"\"
echo $(PATHS) > $(<[1])
\"\"\"
def function(target, sources, ps):
    bjam.call('set-target-variable', target, "PATHS", ps.get('dll-path'))

get_manager().engine().register_action("dll_paths.list", command,
    function=function)
""")

t.write("a/jamfile.jam", "lib a : a.cpp ;")
t.write("a/a.cpp", """\
void
#if defined(_WIN32)
__declspec(dllexport)
#endif
foo() {}
""")

t.write("b/jamfile.jam", "lib b : b.cpp ../a//a ;")
t.write("b/b.cpp", """\
void
#if defined(_WIN32)
__declspec(dllexport)
#endif
bar() {}
""")

t.run_build_system(["hardcode-dll-paths=true"])

t.expect_addition("bin/$toolset/debug/mp.pathlist")

es1 = t.adjust_names("a/bin/$toolset/debug")[0]
es2 = t.adjust_names("b/bin/$toolset/debug")[0]

t.expect_content_lines("bin/$toolset/debug/mp.pathlist", "*" + es1);
t.expect_content_lines("bin/$toolset/debug/mp.pathlist", "*" + es2);

t.cleanup()

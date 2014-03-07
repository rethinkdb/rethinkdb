#!/usr/bin/python

# Copyright Vladimir Prus 2005.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Regression test. When Jamfile contained "using whatever ; " and the 'whatever'
# module declared a project, then all targets in Jamfile were considered to be
# declared in the project associated with 'whatever', not with the Jamfile.

import BoostBuild

t = BoostBuild.Tester(use_test_config=False)

t.write("a.cpp", "int main() {}\n")

t.write("jamroot.jam", """\
using some_tool ;
exe a : a.cpp ;
""")

t.write("some_tool.jam", """\
import project ;
project.initialize $(__name__) ;
rule init ( ) { }
""")

t.write("some_tool.py", """\
from b2.manager import get_manager
get_manager().projects().initialize(__name__)
def init():
    pass
""")

t.run_build_system()
t.expect_addition("bin/$toolset/debug/a.exe")

t.cleanup()

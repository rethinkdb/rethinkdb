#!/usr/bin/python

#  Copyright (C) Vladimir Prus 2006.
#  Distributed under the Boost Software License, Version 1.0. (See
#  accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)

#  Test the 'qt4' examples.

import BoostBuild

t = BoostBuild.Tester()

t.set_tree("../example/qt/qt4/hello")
t.run_build_system()
t.expect_addition(["bin/$toolset/debug/threading-multi/arrow"])

t.set_tree("../example/qt/qt4/moccable-cpp")
t.run_build_system()
t.expect_addition(["bin/$toolset/debug/threading-multi/main"])

t.set_tree("../example/qt/qt4/uic")
t.run_build_system()
t.expect_addition(["bin/$toolset/debug/threading-multi/hello"])

t.cleanup()

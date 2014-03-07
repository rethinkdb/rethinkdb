#!/usr/bin/python

# Copyright 2003 Dave Abrahams
# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Test the 'symlink' rule.

import os
import BoostBuild


if os.name != 'posix':
    print "The symlink tests can be run on posix only."
    import sys
    sys.exit(1)


t = BoostBuild.Tester(use_test_config=False)

t.write("jamroot.jam", "import gcc ;")

t.write("jamfile.jam", """
exe hello : hello.cpp ;
symlink hello_release : hello/<variant>release ;
symlink hello_debug : hello/<variant>debug ;
symlink links/hello_release : hello/<variant>release ;
""")

t.write("hello.cpp", """
int main() {}
""")

t.run_build_system()
t.expect_addition([
    'hello_debug.exe',
    'hello_release.exe',
    'links/hello_release.exe'])

t.cleanup()

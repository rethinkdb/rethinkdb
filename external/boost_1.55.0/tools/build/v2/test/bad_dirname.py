#!/usr/bin/python

# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# Regression test: when directory of project root contained regex
# metacharacters, Boost.Build failed to work. Bug reported by Michael Stevens.

import BoostBuild

t = BoostBuild.Tester()

t.write("bad[abc]dirname/jamfile.jam", """
""")

t.write("bad[abc]dirname/jamroot.jam", """
""")

t.run_build_system(subdir="bad[abc]dirname")

t.cleanup()

#!/usr/bin/python

# Copyright 2003 Vladimir Prus
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

t = BoostBuild.Tester()

# Regression test for double loading of the same Jamfile.
t.write("jamroot.jam", "")
t.write("jamfile.jam", "build-project subdir ;")
t.write("subdir/jamfile.jam", 'ECHO "Loaded subdir" ;')

t.run_build_system(subdir="subdir")
t.expect_output_lines("Loaded subdir")


# Regression test for a more contrived case. The top-level Jamfile refers to
# subdir via use-project, while subdir's Jamfile is being loaded. The
# motivation why use-project referring to subprojects is useful can be found
# at: http://article.gmane.org/gmane.comp.lib.boost.build/3906
t.write("jamroot.jam", "")
t.write("jamfile.jam", "use-project /subdir : subdir ;")
t.write("subdir/jamfile.jam", "project subdir ;")

t.run_build_system(subdir="subdir");

t.cleanup()

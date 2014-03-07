#!/usr/bin/python

# Copyright (C) FILL SOMETHING HERE 2006.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# This file is template for Boost.Build tests. It creates a simple project that
# builds one exe from one source, and checks that the exe is really created.

import BoostBuild


# Create a temporary working directory.
t = BoostBuild.Tester()

# Create the needed files.
t.write("jamroot.jam", """
exe hello : hello.cpp ;
""")

t.write("hello.cpp", """
int main() {}
"""

# Run the build.
t.run_build_system()

# First, create a list of three pathnames.
file_list = BoostBuild.List("bin/$toolset/debug/") * \
    BoostBuild.List("hello.exe hello.obj")
# Second, assert that those files were added as result of the last build system
# invocation.
t.expect_addition(file_list)

# Invoke the build system once again.
t.run_build_system("clean")
# Check if the files added previously were removed.
t.expect_removal(file_list)

# Remove temporary directories.
t.cleanup()

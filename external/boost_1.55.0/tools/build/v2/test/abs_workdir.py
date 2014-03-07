# Niklaus Giger, 2005-03-15
# Testing whether we may run a test in absolute directories. There are no tests
# for temporary directories as this is implictly tested in a lot of other cases.

# TODO: Move to a separate testing-system test group.
# TODO: Make the test not display any output on success.
# TODO: Make sure implemented path handling is correct under Windows, Cygwin &
#       Unix/Linux.

import BoostBuild
import os
import tempfile

t = BoostBuild.Tester(["-ffile.jam"], workdir=os.getcwd(), pass_d0=False,
    pass_toolset=False)

t.write("file.jam", "EXIT [ PWD ] : 0 ;")

t.run_build_system()
t.expect_output_lines("*%s*" % tempfile.gettempdir(), False)
t.expect_output_lines("*build/v2/test*")

t.run_build_system(status=1, subdir="/must/fail/with/absolute/path",
    stderr=None)

t.cleanup()

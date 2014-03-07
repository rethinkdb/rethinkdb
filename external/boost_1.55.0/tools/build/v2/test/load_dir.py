#!/usr/bin/python

"""
Traverses a directory and output the code that would create the same directory
structure during testing. Assumes that the instance of Tester is called 't'.
"""

import sys
import os
import stat
import string

def usage():
    print "usage: load_dir.py directory"


def remove_first_component(path):
    result = [path]
    while 1:
        s = os.path.split(result[0])
        if not s[0]:
            break
        result[:1] = list(s)
    return apply(os.path.join, result[1:])


def create_file(arg, dirname, fnames):
    for n in fnames:
        path = os.path.join(dirname, n)
        if not os.path.isdir(path):
            print "t.write(\"%s\", \"\"\"" % (remove_first_component(path),),
            f = open(path, "r")
            for l in f:
                print l,
            print '\n""")\n'


header = """#!/usr/bin/python

#  Copyright (C) FILL SOMETHING HERE 2005.
#  Distributed under the Boost Software License, Version 1.0. (See
#  accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)

import BoostBuild

t = BoostBuild.Tester()
"""

footer = """

t.run_build_system()

t.expect_addition("bin/$toolset/debug/FILL_SOME_HERE.exe")

t.cleanup()
"""


def main():
    if len(sys.argv) != 2:
        usage()
    else:
        path = sys.argv[1]

        if not os.access(path, os.F_OK):
            print "Path '%s' does not exist" % (path,)
            sys.exit(1)

        if not os.path.isdir(path):
            print "Path '%s' is not a directory" % (path,)

        print header

        os.path.walk(path, create_file, None)

        print footer


if __name__ == '__main__':
    main()

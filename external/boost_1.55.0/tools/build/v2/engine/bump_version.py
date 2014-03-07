#!/usr/bin/python

# This script is used to bump the bjam version. It takes a single argument, e.g
#
#    ./bump_version.py 3.1.9
#
# and updates all the necessary files.
#
# Copyright 2006 Rene Rivera.
# Copyright 2005-2006 Vladimir Prus.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)


import os
import os.path
import re
import string
import sys

srcdir = os.path.abspath(os.path.dirname(__file__))
docdir = os.path.abspath(os.path.join(srcdir, "..", "doc"))


def edit(file, *replacements):
    print("  '%s'..." % file)
    f = open(file, 'r')
    text = f.read()
    f.close()
    for (source, target) in replacements:
        text, n = re.compile(source, re.MULTILINE).subn(target, text)
        assert n > 0
    f = open(file, 'w')
    f.write(text)
    f.close()


def make_edits(ver):
    ver03 = (list(ver) + [0] * 3)[0:3]
    ver02 = ver03[0:2]

    join = lambda v, s : s.join(str(x) for x in v)
    dotJoin = lambda v : join(v, ".")

    print("Setting version to %s" % str(ver03))

    edit(os.path.join(srcdir, "boost-jam.spec"),
        ('^(Version:) .*$', '\\1 %s' % dotJoin(ver03)))

    edit(os.path.join(srcdir, "build.jam"),
        ('^(_VERSION_ =).* ;$', '\\1 %s ;' % join(ver03, " ")))

    edit(os.path.join(docdir, "bjam.qbk"),
        ('(\[version).*(\])', '\\1: %s\\2' % dotJoin(ver03)),
        ('(\[def :version:).*(\])', '\\1 %s\\2' % dotJoin(ver03)))

    edit(os.path.join(srcdir, "patchlevel.h"),
        ('^(#define VERSION_MAJOR) .*$', '\\1 %s' % ver03[0]),
        ('^(#define VERSION_MINOR) .*$', '\\1 %s' % ver03[1]),
        ('^(#define VERSION_PATCH) .*$', '\\1 %s' % ver03[2]),
        ('^(#define VERSION_MAJOR_SYM) .*$', '\\1 "%02d"' % ver03[0]),
        ('^(#define VERSION_MINOR_SYM) .*$', '\\1 "%02d"' % ver03[1]),
        ('^(#define VERSION_PATCH_SYM) .*$', '\\1 "%02d"' % ver03[2]),
        ('^(#define VERSION) .*$', '\\1 "%s"' % dotJoin(ver)),
        ('^(#define JAMVERSYM) .*$', '\\1 "JAMVERSION=%s"' % dotJoin(ver02)))


def main():
    if len(sys.argv) < 2:
        print("Expect new version as argument.")
        sys.exit(1)
    if len(sys.argv) > 3:
        print("Too many arguments.")
        sys.exit(1)

    version = sys.argv[1].split(".")
    if len(version) > 3:
        print("Expect version argument in the format: <MAJOR>.<MINOR>.<PATCH>")
        sys.exit(1)

    try:
        version = list(int(x) for x in version)
    except ValueError:
        print("Version values must be valid integers.")
        sys.exit(1)

    while version and version[-1] == 0:
        version.pop()

    if not version:
        print("At least one of the version values must be positive.")
        sys.exit()

    make_edits(version)


if __name__ == '__main__':
    main()

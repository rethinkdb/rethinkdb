# Copyright David Abrahams 2004. Use, modification and distribution is
# subject to the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# This script accepts a list of .rst files to be processed and
# generates Makefile dependencies for .html and .rst files to stdout.
import os,sys
import re

include = re.compile(r' *\.\. +(include|image):: +(.*)', re.MULTILINE)

def deps(path, found):
    dir = os.path.split(path)[0]
    for m in re.findall(include, open(path).read()):

        dependency = os.path.normpath(os.path.join(dir,m[1]))
        if dependency not in found:
            found[dependency] = 1

            if m[0] == 'include':
                deps(dependency, found)
                
    return found
                
for file in sys.argv[1:]:
    found = deps(file, {})
    if found:
        base = os.path.splitext(os.path.basename(file))[0]
        print '%s.tex %s.html: %s' % (base, base, ' '.join(found.keys()))

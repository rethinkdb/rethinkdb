#!/usr/bin/python
# Copyright David Abrahams 2004. Use, modification and distribution is
# subject to the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#
# Generate html, TeX, and PDF versions of all the source files
#
import os
import sys

from syscmd import syscmd
from sources import sources

if 0:
    for s in sources:
        syscmd('boosthtml %s' % s)
else:        
    extensions = ('html', 'pdf')

    if len(sys.argv) > 1:
        extensions = sys.argv[1:]

    all = [ '%s.%s' % (os.path.splitext(s)[0],ext)
          for ext in extensions
          for s in sources 
        ]

    print 'make %s' % ' '.join(all)
    syscmd('make %s' % ' '.join(all))



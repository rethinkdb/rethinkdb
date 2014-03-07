# Copyright David Abrahams 2004. Use, modification and distribution is
# subject to the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import os
import sys

def syscmd(s):
    print 'executing: ', repr(s)
    sys.stdout.flush()
    err = os.system(s)
    if err:
        raise SystemError, 'command: %s returned %s' % (
            repr(s), err)

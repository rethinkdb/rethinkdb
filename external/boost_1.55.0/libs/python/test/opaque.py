# -*- coding: latin-1 -*-
# Copyright Gottfried Ganßauge 2003..2006.  Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


"""
>>> from opaque_ext import *


    Check for correct conversion
    
>>> use(get())

    Check that None is converted to a NULL opaque pointer
    
>>> useany(get())
1
>>> useany(None)
0

    Check that we don't lose type information by converting NULL
    opaque pointers to None
    
>>> assert getnull() is None
>>> useany(getnull())
0

>>> failuse(get())
Traceback (most recent call last):
        ...
RuntimeError: success

   Check that there is no conversion from integers ...
   
>>> try: use(0)
... except TypeError: pass
... else: print 'expected a TypeError'

   ... and from strings to opaque objects
   
>>> try: use("")
... except TypeError: pass
... else: print 'expected a TypeError'

   Now check the same for another opaque pointer type
   
>>> use2(get2())
>>> failuse2(get2())
Traceback (most recent call last):
        ...
RuntimeError: success
>>> try: use2(0)
... except TypeError: pass
... else: print 'expected a TypeError'
>>> try: use2("")
... except TypeError: pass
... else: print 'expected a TypeError'

   Check that opaque types are distinct

>>> try: use(get2())
... except TypeError: pass
... else: print 'expected a TypeError'
>>> try: use2(get())
... except TypeError: pass
... else: print 'expected a TypeError'

   This used to result in a segmentation violation

>>> type(get()) != type (get2())
1
"""
def run(args = None):
    import sys
    import doctest

    if args is not None:
        sys.argv = args
    return doctest.testmod(sys.modules.get(__name__))
    
if __name__ == '__main__':
    print "running..."
    import sys
    status = run()[0]
    if (status == 0): print "Done."
    sys.exit(status)

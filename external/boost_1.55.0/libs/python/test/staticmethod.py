# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
>>> from staticmethod_ext import *

>>> class X1(X):
...     pass


>>> x = X(16)
>>> x1 = X1(17)



>>> x1.count()
2

>>> x.count()
2

>>> X1.count()
2

>>> X.count()
2


>>> x1.magic()
7654321

>>> x.magic()
7654321

>>> X1.magic()
7654321

>>> X.magic()
7654321


'''

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

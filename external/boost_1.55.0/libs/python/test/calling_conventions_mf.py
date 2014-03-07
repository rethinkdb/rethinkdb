# Copyright Nicolas Lelong,  2010. Distributed under the Boost 
# Software License, Version 1.0 (See accompanying 
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
"""
>>> from calling_conventions_mf_ext import *
>>> x = X__cdecl()
>>> x.f0()
>>> x.g0()
>>> x.f1(1)
>>> x.g1(1)
>>> x.f2(1, 2)
>>> x.g2(1, 2)
>>> x.f3(1, 2, 3)
>>> x.g3(1, 2, 3)
>>> x.f4(1, 2, 3, 4)
>>> x.g4(1, 2, 3, 4)
>>> x.f5(1, 2, 3, 4, 5)
>>> x.g5(1, 2, 3, 4, 5)
>>> x.f6(1, 2, 3, 4, 5, 6)
>>> x.g6(1, 2, 3, 4, 5, 6)
>>> x.f7(1, 2, 3, 4, 5, 6, 7)
>>> x.g7(1, 2, 3, 4, 5, 6, 7)
>>> x.f8(1, 2, 3, 4, 5, 6, 7, 8)
>>> x.g8(1, 2, 3, 4, 5, 6, 7, 8)
>>> x.hash
2155
>>> x = X__stdcall()
>>> x.f0()
>>> x.g0()
>>> x.f1(1)
>>> x.g1(1)
>>> x.f2(1, 2)
>>> x.g2(1, 2)
>>> x.f3(1, 2, 3)
>>> x.g3(1, 2, 3)
>>> x.f4(1, 2, 3, 4)
>>> x.g4(1, 2, 3, 4)
>>> x.f5(1, 2, 3, 4, 5)
>>> x.g5(1, 2, 3, 4, 5)
>>> x.f6(1, 2, 3, 4, 5, 6)
>>> x.g6(1, 2, 3, 4, 5, 6)
>>> x.f7(1, 2, 3, 4, 5, 6, 7)
>>> x.g7(1, 2, 3, 4, 5, 6, 7)
>>> x.f8(1, 2, 3, 4, 5, 6, 7, 8)
>>> x.g8(1, 2, 3, 4, 5, 6, 7, 8)
>>> x.hash
2155
>>> x = X__fastcall()
>>> x.f0()
>>> x.g0()
>>> x.f1(1)
>>> x.g1(1)
>>> x.f2(1, 2)
>>> x.g2(1, 2)
>>> x.f3(1, 2, 3)
>>> x.g3(1, 2, 3)
>>> x.f4(1, 2, 3, 4)
>>> x.g4(1, 2, 3, 4)
>>> x.f5(1, 2, 3, 4, 5)
>>> x.g5(1, 2, 3, 4, 5)
>>> x.f6(1, 2, 3, 4, 5, 6)
>>> x.g6(1, 2, 3, 4, 5, 6)
>>> x.f7(1, 2, 3, 4, 5, 6, 7)
>>> x.g7(1, 2, 3, 4, 5, 6, 7)
>>> x.f8(1, 2, 3, 4, 5, 6, 7, 8)
>>> x.g8(1, 2, 3, 4, 5, 6, 7, 8)
>>> x.hash
2155
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

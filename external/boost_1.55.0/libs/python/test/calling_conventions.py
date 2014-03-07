# Copyright Nicolas Lelong,  2010. Distributed under the Boost 
# Software License, Version 1.0 (See accompanying 
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
"""
>>> from calling_conventions_ext import *
>>> f_0__cdecl()
17041
>>> f_1__cdecl(1)
1
>>> f_2__cdecl(1, 2)
21
>>> f_3__cdecl(1, 2, 3)
321
>>> f_4__cdecl(1, 2, 3, 4)
4321
>>> f_5__cdecl(1, 2, 3, 4, 5)
54321
>>> f_6__cdecl(1, 2, 3, 4, 5, 6)
654321
>>> f_7__cdecl(1, 2, 3, 4, 5, 6, 7)
7654321
>>> f_8__cdecl(1, 2, 3, 4, 5, 6, 7, 8)
87654321
>>> f_9__cdecl(1, 2, 3, 4, 5, 6, 7, 8, 9)
987654321
>>> f_0__stdcall()
17041
>>> f_1__stdcall(1)
1
>>> f_2__stdcall(1, 2)
21
>>> f_3__stdcall(1, 2, 3)
321
>>> f_4__stdcall(1, 2, 3, 4)
4321
>>> f_5__stdcall(1, 2, 3, 4, 5)
54321
>>> f_6__stdcall(1, 2, 3, 4, 5, 6)
654321
>>> f_7__stdcall(1, 2, 3, 4, 5, 6, 7)
7654321
>>> f_8__stdcall(1, 2, 3, 4, 5, 6, 7, 8)
87654321
>>> f_9__stdcall(1, 2, 3, 4, 5, 6, 7, 8, 9)
987654321
>>> f_0__fastcall()
17041
>>> f_1__fastcall(1)
1
>>> f_2__fastcall(1, 2)
21
>>> f_3__fastcall(1, 2, 3)
321
>>> f_4__fastcall(1, 2, 3, 4)
4321
>>> f_5__fastcall(1, 2, 3, 4, 5)
54321
>>> f_6__fastcall(1, 2, 3, 4, 5, 6)
654321
>>> f_7__fastcall(1, 2, 3, 4, 5, 6, 7)
7654321
>>> f_8__fastcall(1, 2, 3, 4, 5, 6, 7, 8)
87654321
>>> f_9__fastcall(1, 2, 3, 4, 5, 6, 7, 8, 9)
987654321
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

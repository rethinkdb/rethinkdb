# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
>>> from operators_ext import *

  Check __nonzero__ support
  
>>> assert X(2)
>>> assert not X(0)

 ----
 
>>> x = X(42)
>>> x.value()
42
>>> y = x - X(5)
>>> y.value()
37
>>> y = x - 4
>>> y.value()
38
>>> y = 3 - x
>>> y.value()
-39
>>> (-y).value()
39

>>> (x + y).value()
3

>>> abs(y).value()
39

>>> x < 10
0
>>> x < 43
1

>>> 10 < x
1
>>> 43 < x
0

>>> x < y
0
>>> y < x
1

 ------
>>> x > 10
1
>>> x > 43
0

>>> 10 > x
0
>>> 43 > x
1

>>> x > y
1
>>> y > x
0

>>> y = x - 5
>>> x -= y
>>> x.value()
5
>>> str(x)
'5'

>>> z = Z(10)
>>> int(z)
10
>>> float(z)
10.0
>>> complex(z)
(10+0j)

>>> pow(2,x)
32
>>> pow(x,2).value()
25
>>> pow(X(2),x).value()
32
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

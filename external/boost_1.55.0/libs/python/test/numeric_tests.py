# Copyright David Abrahams 2006. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
import printer
'''
>>> from numpy_ext import *
>>> x = new_array()
>>> x[1,1] = 0.0

>>> try: take_array(3)
... except TypeError: pass
... else: print 'expected a TypeError'

>>> take_array(x)

>>> print x
[[1 2 3]
 [4 0 6]
 [7 8 9]]

>>> y = x.copy()


>>> p = _printer()
>>> check = p.check
>>> exercise(x, p)
>>> y[2,1] = 3
>>> check(y);

>>> check(y.astype('D'));

>>> check(y.copy());

>>> check(y.typecode());

>>> p.results
[]
>>> del p
'''

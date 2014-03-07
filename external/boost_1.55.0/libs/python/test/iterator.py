# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
>>> from iterator_ext import *
>>> from input_iterator import *
>>> x = list_int()
>>> x.push_back(1)
>>> x.back()
1
>>> x.push_back(3)
>>> x.push_back(5)
>>> for y in x:
...     print y
1
3
5
>>> z = range(x)
>>> for y in z:
...     print y
1
3
5

   Range2 wraps a transform_iterator which doubles the elements it
   traverses. This proves we can wrap input iterators
   
>>> z2 = range2(x)
>>> for y in z2:
...     print y
2
6
10

>>> l2 = two_lists()
>>> for y in l2.primes:
...     print y
2
3
5
7
11
13
>>> for y in l2.evens:
...     print y
2
4
6
8
10
12
>>> ll = list_list()
>>> ll.push_back(x)
>>> x.push_back(7)
>>> ll.push_back(x)
>>> for a in ll: #doctest: +NORMALIZE_WHITESPACE
...     for b in a:
...         print b,
...     print
...
1 3 5
1 3 5 7
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

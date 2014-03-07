# Copyright Eric Niebler 2005. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
>>> from stl_iterator_ext import *
>>> x = list_int()
>>> x.assign(iter([1,2,3,4,5]))
>>> for y in x:
...     print y
1
2
3
4
5
>>> def generator():
...   yield 1
...   yield 2
...   raise RuntimeError, "oops"
>>> try:
...   x.assign(iter(generator()))
...   print "NOT OK"
... except RuntimeError:
...   print "OK"
OK
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

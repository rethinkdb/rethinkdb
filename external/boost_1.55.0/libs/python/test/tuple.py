# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
"""
>>> from tuple_ext import *
>>> def printer(*args):
...     for x in args: print x,
...     print
...
>>> print convert_to_tuple("this is a test string")
('t', 'h', 'i', 's', ' ', 'i', 's', ' ', 'a', ' ', 't', 'e', 's', 't', ' ', 's', 't', 'r', 'i', 'n', 'g')
>>> t1 = convert_to_tuple("this is")
>>> t2 = (1,2,3,4)
>>> test_operators(t1,t2,printer) #doctest: +NORMALIZE_WHITESPACE
('t', 'h', 'i', 's', ' ', 'i', 's', 1, 2, 3, 4)
>>> make_tuple()
()
>>> make_tuple(42)
(42,)
>>> make_tuple('hello', 42)
('hello', 42)
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

# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
>>> from auto_ptr_ext import *
>>> x = X(42)
>>> x.value()
42
>>> look(x), look(x)
(42, 42)

>>> maybe_steal(x, 0)
42
>>> look(x)
42

>>> maybe_steal(x, 1)
42
>>> broken_auto_ptr and -1 or look(x)
-1

>>> x = X(69)
>>> steal(x)
69
>>> broken_auto_ptr and -1 or look(x)
-1

>>> if not broken_auto_ptr:
...     try: x.value()
...     except TypeError: pass
...     else: print 'expected a TypeError exception'

>>> x = make()
>>> look(x)
77

>>> z = callback(lambda z: z)
>>> z.value()
77

>>> extract(x).value()
77

#
# Test derived to base conversions
#

>>> y = Y(42)
>>> y.value()
42

>>> try: maybe_steal(y, 0)
... except TypeError: pass
... else: print 'expected a TypeError exception'

>>> y.value()
42

>>> broken_auto_ptr and 42 or steal(y)
42

>>> if not broken_auto_ptr:
...     try: y.value()
...     except TypeError: pass
...     else: print 'expected a TypeError exception'

>>> print look.__doc__.splitlines()[1]
look( (X)arg1) -> int :

>>> print steal.__doc__.splitlines()[1]
steal( (X)arg1) -> int :

>>> print maybe_steal.__doc__.splitlines()[1]
maybe_steal( (X)arg1, (bool)arg2) -> int :

>>> print make.__doc__.splitlines()[1]
make() -> X :

>>> print callback.__doc__.splitlines()[1]
callback( (object)arg1) -> X :

>>> print extract.__doc__.splitlines()[1]
extract( (object)arg1) -> X :

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

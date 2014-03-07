# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
"""
>>> from args_ext import *

>>> raw(3, 4, foo = 'bar', baz = 42)
((3, 4), {'foo': 'bar', 'baz': 42})

   Prove that we can handle empty keywords and non-keywords
   
>>> raw(3, 4)
((3, 4), {})

>>> raw(foo = 'bar')
((), {'foo': 'bar'})

>>> f(x= 1, y = 3, z = 'hello')
(1, 3.0, 'hello')

>>> f(z = 'hello', x = 3, y = 2.5)
(3, 2.5, 'hello')

>>> f(1, z = 'hi', y = 3)
(1, 3.0, 'hi')

>>> try: f(1, 2, 'hello', bar = 'baz')
... except TypeError: pass
... else: print 'expected an exception: unknown keyword'


   Exercise the functions using default stubs

>>> f1(z = 'nix', y = .125, x = 2)
(2, 0.125, 'nix')
>>> f1(y = .125, x = 2)
(2, 0.125, 'wow')
>>> f1(x = 2)
(2, 4.25, 'wow')
>>> f1()
(1, 4.25, 'wow')

>>> f2(z = 'nix', y = .125, x = 2)
(2, 0.125, 'nix')
>>> f2(y = .125, x = 2)
(2, 0.125, 'wow')
>>> f2(x = 2)
(2, 4.25, 'wow')
>>> f2()
(1, 4.25, 'wow')

>>> f3(z = 'nix', y = .125, x = 2)
(2, 0.125, 'nix')
>>> f3(y = .125, x = 2)
(2, 0.125, 'wow')
>>> f3(x = 2)
(2, 4.25, 'wow')
>>> f3()
(1, 4.25, 'wow')

   Member function tests

>>> q = X()
>>> q.f(x= 1, y = 3, z = 'hello')
(1, 3.0, 'hello')

>>> q.f(z = 'hello', x = 3, y = 2.5)
(3, 2.5, 'hello')

>>> q.f(1, z = 'hi', y = 3)
(1, 3.0, 'hi')

>>> try: q.f(1, 2, 'hello', bar = 'baz')
... except TypeError: pass
... else: print 'expected an exception: unknown keyword'

   Exercise member functions using default stubs
   
>>> q.f1(z = 'nix', y = .125, x = 2)
(2, 0.125, 'nix')
>>> q.f1(y = .125, x = 2)
(2, 0.125, 'wow')
>>> q.f1(x = 2)
(2, 4.25, 'wow')
>>> q.f1()
(1, 4.25, 'wow')
>>> q.f2.__doc__.splitlines()[1]
'f2( (X)self [, (int)x [, (float)y [, (str)z]]]) -> tuple :'

>>> q.f2.__doc__.splitlines()[2]
"    f2's docstring"

>>> X.f.__doc__.splitlines()[1:5]
['f( (X)self, (int)x, (float)y, (str)z) -> tuple :', "    This is X.f's docstring", '', '    C++ signature :']

>>> xfuncs = (X.inner0, X.inner1, X.inner2, X.inner3, X.inner4, X.inner5)
>>> for f in xfuncs:
...    print f(q,1).value(),
...    print f(q, n = 1).value(),
...    print f(q, n = 0).value(),
...    print f.__doc__.splitlines()[1:5]
1 1 0 ['inner0( (X)self, (bool)n) -> Y :', '    docstring', '', '    C++ signature :']
1 1 0 ['inner1( (X)self, (bool)n) -> Y :', '    docstring', '', '    C++ signature :']
1 1 0 ['inner2( (X)self, (bool)n) -> Y :', '    docstring', '', '    C++ signature :']
1 1 0 ['inner3( (X)self, (bool)n) -> Y :', '    docstring', '', '    C++ signature :']
1 1 0 ['inner4( (X)self, (bool)n) -> Y :', '    docstring', '', '    C++ signature :']
1 1 0 ['inner5( (X)self, (bool)n) -> Y :', '    docstring', '', '    C++ signature :']

>>> x = X(a1 = 44, a0 = 22)
>>> x.inner0(0).value()
22
>>> x.inner0(1).value()
44

>>> x = X(a0 = 7)
>>> x.inner0(0).value()
7
>>> x.inner0(1).value()
1

>>> inner(n = 1, self = q).value()
1

>>> y = Y(value = 33)
>>> y.raw(this = 1, that = 'the other')[1]
{'this': 1, 'that': 'the other'}

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
    import args_ext
    help(args_ext)
    sys.exit(status)




# Copyright Daniel Wallin 2006. Distributed under the
# Boost Software License, Version 1.0. (See accompanying file
# LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

'''
>>> from python_test_ext import X
>>> x = X(y = 'baz')
>>> x.value
'foobaz'
>>> x.f(1,2)
3
>>> x.f(1,2,3)
6
>>> x.f(1,2, z = 3)
6
>>> x.f(z = 3, y = 2, x = 1)
6
>>> x.g()
'foobar'
>>> x.g(y = "baz")
'foobaz'
>>> x.g(x = "baz")
'bazbar'
>>> x.g(y = "foo", x = "bar")
'barfoo'
>>> y = x.h(x = "bar", y = "foo")
>>> assert x == y
>>> y = x(0)
>>> assert x == y
'''

def run(args = None):
    if args is not None:
        import sys
        sys.argv = args
    import doctest, python_test 
    return doctest.testmod(python_test)

if __name__ == '__main__':
    import sys
    sys.exit(run()[0])

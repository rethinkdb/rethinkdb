# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
>>> from list_ext import *

>>> new_list()
[]

>>> listify((1,2,3))
[1, 2, 3]

>>> letters = listify_string('hello')
>>> letters
['h', 'e', 'l', 'l', 'o']

>>> X(22)
X(22)

>>> def identity(x):
...     return x
>>> assert apply_object_list(identity, letters) is letters

  5 is not convertible to a list

>>> try: result = apply_object_list(identity, 5)
... except TypeError: pass
... else: print 'expected an exception, got', result, 'instead'

>>> assert apply_list_list(identity, letters) is letters

  5 is not convertible to a list as a return value

>>> try: result = apply_list_list(len, letters)
... except TypeError: pass
... else: print 'expected an exception, got', result, 'instead'

>>> append_object(letters, '.')
>>> letters
['h', 'e', 'l', 'l', 'o', '.']

  tuples do not automatically convert to lists when passed as arguments
  
>>> try: append_list(letters, (1,2))
... except TypeError: pass
... else: print 'expected an exception'

>>> append_list(letters, [1,2])
>>> letters
['h', 'e', 'l', 'l', 'o', '.', [1, 2]]

    Check that subclass functions are properly called
    
>>> class mylist(list):
...     def append(self, o):
...         list.append(self, o)
...         if not hasattr(self, 'nappends'):
...             self.nappends = 1
...         else:
...             self.nappends += 1
...
>>> l2 = mylist()
>>> append_object(l2, 'hello')
>>> append_object(l2, 'world')
>>> l2
['hello', 'world']
>>> l2.nappends
2

>>> def printer(*args):
...     for x in args: print x,
...     print
...

>>> y = X(42)
>>> exercise(letters, y, printer) #doctest: +NORMALIZE_WHITESPACE
after append:
['h', 'e', 'l', 'l', 'o', '.', [1, 2], X(42), 5, X(3)]
number of X(42) instances: 1
number of 5s: 1
after extend:
['h', 'e', 'l', 'l', 'o', '.', [1, 2], X(42), 5, X(3), 'x', 'y', 'z']
index of X(42) is: 7
index of 'l' is: 2
after inserting 666:
['h', 'e', 'l', 'l', 666, 'o', '.', [1, 2], X(42), 5, X(3), 'x', 'y', 'z']
inserting with object as index:
['h', 'e', 'l', 'l', 666, '---', 'o', '.', [1, 2], X(42), 5, X(3), 'x', 'y', 'z']
popping...
['h', 'e', 'l', 'l', 666, '---', 'o', '.', [1, 2], X(42), 5, X(3), 'x', 'y']
['h', 'e', 'l', 'l', 666, 'o', '.', [1, 2], X(42), 5, X(3), 'x', 'y']
['h', 'e', 'l', 'l', 666, 'o', '.', [1, 2], X(42), X(3), 'x', 'y']
removing X(42)
['h', 'e', 'l', 'l', 666, 'o', '.', [1, 2], X(3), 'x', 'y']
removing 666
['h', 'e', 'l', 'l', 'o', '.', [1, 2], X(3), 'x', 'y']
reversing...
['y', 'x', X(3), [1, 2], '.', 'o', 'l', 'l', 'e', 'h']
sorted:
['.', 'e', 'h', 'l', 'l', 'o', 'x', 'y']
reverse sorted:
['y', 'x', 'o', 'l', 'l', 'h', 'e', '.']
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

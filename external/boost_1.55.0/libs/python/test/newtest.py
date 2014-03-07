# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
"""
>>> from m1 import *

>>> from m2 import *

   Prove that we get an appropriate error from trying to return a type
   for which we have no registered to_python converter

>>> def check_unregistered(f, msgprefix):   
...     try:
...         f(1)
...     except TypeError, x:
...         if not str(x).startswith(msgprefix):
...             print str(x)
...     else:
...         print 'expected a TypeError'
...
>>> check_unregistered(make_unregistered, 'No to_python (by-value) converter found for C++ type')
>>> check_unregistered(make_unregistered2, 'No Python class registered for C++ class')

>>> n = new_noddy()
>>> s = new_simple()
>>> unwrap_int(n)
42
>>> unwrap_int_ref(n)
42
>>> unwrap_int_const_ref(n)
42
>>> unwrap_simple(s)
'hello, world'
>>> unwrap_simple_ref(s)
'hello, world'
>>> unwrap_simple_const_ref(s)
'hello, world'
>>> unwrap_int(5)
5

Can't get a non-const reference to a built-in integer object
>>> try:
...     unwrap_int_ref(7)
... except: pass
... else: print 'no exception'

>>> unwrap_int_const_ref(9)
9

>>> wrap_int(n)
42

try: wrap_int_ref(n)
... except: pass
... else: print 'no exception'

>>> wrap_int_const_ref(n)
42

>>> unwrap_simple_ref(wrap_simple(s))
'hello, world'

>>> unwrap_simple_ref(wrap_simple_ref(s))
'hello, world'

>>> unwrap_simple_ref(wrap_simple_const_ref(s))
'hello, world'

>>> f(s)
12

>>> unwrap_simple(g(s))
'hello, world'

>>> f(g(s))
12

>>> f_mutable_ref(g(s))
12

>>> f_const_ptr(g(s))
12

>>> f_mutable_ptr(g(s))
12

>>> f2(g(s))
12

Create an extension class which wraps "complicated" (init1 and get_n)
are a complicated constructor and member function, respectively.

>>> c1 = complicated(s, 99)
>>> c1.get_n()
99
>>> c2 = complicated(s)
>>> c2.get_n()
0

     a quick regression test for a bug where None could be converted
     to the target of any member function. To see it, we need to
     access the __dict__ directly, to bypass the type check supplied
     by the Method property which wraps the method when accessed as an
     attribute. 

>>> try: A.__dict__['name'](None)
... except TypeError: pass
... else: print 'expected an exception!'


>>> a = A()
>>> b = B()
>>> c = C()
>>> d = D()


>>> take_a(a).name()
'A'

>>> try:
...     take_b(a)
... except: pass
... else: print 'no exception'

>>> try:
...     take_c(a)
... except: pass
... else: print 'no exception'

>>> try:
...     take_d(a)
... except: pass
... else: print 'no exception'

------
>>> take_a(b).name()
'A'

>>> take_b(b).name()
'B'

>>> try:
...     take_c(b)
... except: pass
... else: print 'no exception'

>>> try:
...     take_d(b)
... except: pass
... else: print 'no exception'

-------
>>> take_a(c).name()
'A'

>>> try:
...     take_b(c)
... except: pass
... else: print 'no exception'

>>> take_c(c).name()
'C'

>>> try:
...     take_d(c)
... except: pass
... else: print 'no exception'

-------
>>> take_a(d).name()
'A'
>>> take_b(d).name()
'B'
>>> take_c(d).name()
'C'
>>> take_d(d).name()
'D'

>>> take_d_shared_ptr(d).name()
'D'

>>> d_as_a = d_factory()
>>> dd = take_d(d_as_a)
>>> dd.name()
'D'
>>> print g.__doc__.splitlines()[1]
g( (Simple)arg1) -> Simple :

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

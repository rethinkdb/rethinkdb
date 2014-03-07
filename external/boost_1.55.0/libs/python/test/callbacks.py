# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
>>> from callbacks_ext import *

>>> def double(x):
...     return x + x
...
>>> apply_int_int(double, 42)
84
>>> apply_void_int(double, 42)

>>> def identity(x):
...     return x

Once we have array conversion support, this test will fail. Er,
succeed<wink>:

>>> try: apply_to_string_literal(identity)
... except ReferenceError: pass # expected
... else: print 'expected an exception!'

>>> try: apply_X_ref_handle(lambda ignored:X(42), None)
... except ReferenceError: pass # expected
... else: print 'expected an exception!'

>>> x = X(42)
>>> x.y = X(7)
>>> apply_X_ref_handle(lambda z:z.y, x).value()
7

>>> x = apply_X_X(identity, X(42))
>>> x.value()
42
>>> x_count()
1
>>> del x
>>> x_count()
0

>>> def increment(x):
...     x.set(x.value() + 1)
...
>>> x = X(42)
>>> apply_void_X_ref(increment, x)
>>> x.value()
43

>>> apply_void_X_cref(increment, x) 
>>> x.value()  # const-ness is not respected, sorry!
44

>>> last_x = 1
>>> def decrement(x):
...     global last_x
...     last_x = x
...     if x is not None:
...         x.set(x.value() - 1)

>>> apply_void_X_ptr(decrement, x)
>>> x.value()
43
>>> last_x.value()
43
>>> increment(last_x)
>>> x.value()
44
>>> last_x.value()
44

>>> apply_void_X_ptr(decrement, None)
>>> assert last_x is None
>>> x.value()
44

>>> last_x = 1
>>> apply_void_X_deep_ptr(decrement, None)
>>> assert last_x is None
>>> x.value()
44

>>> apply_void_X_deep_ptr(decrement, x)
>>> x.value()
44
>>> last_x.value()
43

>>> y = apply_X_ref_handle(identity, x)
>>> assert y.value() == x.value()
>>> increment(x)
>>> assert y.value() == x.value()

>>> y = apply_X_ptr_handle_cref(identity, x)
>>> assert y.value() == x.value()
>>> increment(x)
>>> assert y.value() == x.value()

>>> y = apply_X_ptr_handle_cref(identity, None)
>>> y

>>> def new_x(ignored):
...     return X(666)
...
>>> try: apply_X_ref_handle(new_x, 1)
... except ReferenceError: pass
... else: print 'no error'

>>> try: apply_X_ptr_handle_cref(new_x, 1)
... except ReferenceError: pass
... else: print 'no error'

>>> try: apply_cstring_cstring(identity, 'hello')
... except ReferenceError: pass
... else: print 'no error'

>>> apply_char_char(identity, 'x')
'x'

>>> apply_cstring_pyobject(identity, 'hello')
'hello'

>>> apply_cstring_pyobject(identity, None)


>>> apply_char_char(identity, 'x')
'x'

>>> assert apply_to_own_type(identity) is type(identity)

>>> assert apply_object_object(identity, identity) is identity
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

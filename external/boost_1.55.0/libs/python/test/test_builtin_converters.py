# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
r"""
>>> from builtin_converters_ext import *

# Provide values for integer converter tests
>>> def _signed_values(s):
...     base = 2 ** (8 * s - 1)
...     return [[-base, -1, 1, base - 1], [-base - 1, base]]
>>> def _unsigned_values(s):
...     base = 2 ** (8 * s)
...     return [[1, base - 1], [-1L, -1, base]]

# Wrappers to simplify tests
>>> def should_pass(method, values):
...     result = map(method, values[0])
...     if result != values[0]:
...         print "Got %s but expected %s" % (result, values[0])
>>> def test_overflow(method, values):
...     for v in values[1]:
...         try: method(v)
...         except OverflowError: pass
...         else: print "OverflowError expected"

# Synthesize idendity functions in case long long not supported
>>> if not 'rewrap_value_long_long' in dir():
...     def rewrap_value_long_long(x): return long(x)
...     def rewrap_value_unsigned_long_long(x): return long(x)
...     def rewrap_const_reference_long_long(x): return long(x)
...     def rewrap_const_reference_unsigned_long_long(x): return long(x)
>>> if not 'long_long_size' in dir():
...     def long_long_size(): return long_size()

>>> try: bool_exists = bool
... except: pass
... else:
...     rewrap_value_bool(True)
...     rewrap_value_bool(False)
True
False

>>> rewrap_value_bool(None)
0
>>> rewrap_value_bool(0)
0
>>> rewrap_value_bool(33)
1
>>> rewrap_value_char('x')
'x'

  Note that there's currently silent truncation of strings passed to
  char arguments.

>>> rewrap_value_char('xy')
'x'
>>> rewrap_value_signed_char(42)
42
>>> rewrap_value_unsigned_char(42)
42
>>> rewrap_value_int(42)
42
>>> rewrap_value_unsigned_int(42)
42
>>> rewrap_value_short(42)
42
>>> rewrap_value_unsigned_short(42)
42
>>> rewrap_value_long(42)
42
>>> rewrap_value_unsigned_long(42)
42

    test unsigned long values which don't fit in a signed long.
    strip any 'L' characters in case the platform has > 32 bit longs
        
>>> hex(rewrap_value_unsigned_long(0x80000001L)).replace('L','')
'0x80000001'

>>> rewrap_value_long_long(42) == 42
True
>>> rewrap_value_unsigned_long_long(42) == 42
True

   show that we have range checking. 

>>> should_pass(rewrap_value_signed_char, _signed_values(char_size()))
>>> should_pass(rewrap_value_short, _signed_values(short_size()))
>>> should_pass(rewrap_value_int, _signed_values(int_size()))
>>> should_pass(rewrap_value_long, _signed_values(long_size()))
>>> should_pass(rewrap_value_long_long, _signed_values(long_long_size()))

>>> should_pass(rewrap_value_unsigned_char, _unsigned_values(char_size()))
>>> should_pass(rewrap_value_unsigned_short, _unsigned_values(short_size()))
>>> should_pass(rewrap_value_unsigned_int, _unsigned_values(int_size()))
>>> should_pass(rewrap_value_unsigned_long, _unsigned_values(long_size()))
>>> should_pass(rewrap_value_unsigned_long_long,
...     _unsigned_values(long_long_size()))

>>> test_overflow(rewrap_value_signed_char, _signed_values(char_size()))
>>> test_overflow(rewrap_value_short, _signed_values(short_size()))
>>> test_overflow(rewrap_value_int, _signed_values(int_size()))
>>> test_overflow(rewrap_value_long, _signed_values(long_size()))
>>> test_overflow(rewrap_value_long_long, _signed_values(long_long_size()))

>>> test_overflow(rewrap_value_unsigned_char, _unsigned_values(char_size()))
>>> test_overflow(rewrap_value_unsigned_short, _unsigned_values(short_size()))
>>> test_overflow(rewrap_value_unsigned_int, _unsigned_values(int_size()))
>>> test_overflow(rewrap_value_unsigned_long, _unsigned_values(long_size()))

# Exceptionally for PyLong_AsUnsignedLongLong(), a negative value raises
# TypeError on Python versions prior to 2.7
>>> for v in _unsigned_values(long_long_size())[1]:
...     try: rewrap_value_unsigned_long_long(v)
...     except (OverflowError, TypeError): pass
...     else: print "OverflowError or TypeError expected"

>>> assert abs(rewrap_value_float(4.2) - 4.2) < .000001
>>> rewrap_value_double(4.2) - 4.2
0.0
>>> rewrap_value_long_double(4.2) - 4.2
0.0

>>> assert abs(rewrap_value_complex_float(4+.2j) - (4+.2j)) < .000001
>>> assert abs(rewrap_value_complex_double(4+.2j) - (4+.2j)) < .000001
>>> assert abs(rewrap_value_complex_long_double(4+.2j) - (4+.2j)) < .000001

>>> rewrap_value_cstring('hello, world')
'hello, world'
>>> rewrap_value_string('yo, wassup?')
'yo, wassup?'

>>> print rewrap_value_wstring(u'yo, wassup?')
yo, wassup?

   test that overloading on unicode works:

>>> print rewrap_value_string(u'yo, wassup?')
yo, wassup?

   wrap strings with embedded nulls:
   
>>> rewrap_value_string('yo,\0wassup?')
'yo,\x00wassup?'

>>> rewrap_value_handle(1)
1
>>> x = 'hi'
>>> assert rewrap_value_handle(x) is x
>>> assert rewrap_value_object(x) is x

  Note that we can currently get a mutable pointer into an immutable
  Python string:
  
>>> rewrap_value_mutable_cstring('hello, world')
'hello, world'

>>> rewrap_const_reference_bool(None)
0
>>> rewrap_const_reference_bool(0)
0

>>> try: rewrap_const_reference_bool('yes')
... except TypeError: pass
... else: print 'expected a TypeError exception'

>>> rewrap_const_reference_char('x')
'x'

  Note that there's currently silent truncation of strings passed to
  char arguments.

>>> rewrap_const_reference_char('xy')
'x'
>>> rewrap_const_reference_signed_char(42)
42
>>> rewrap_const_reference_unsigned_char(42)
42
>>> rewrap_const_reference_int(42)
42
>>> rewrap_const_reference_unsigned_int(42)
42
>>> rewrap_const_reference_short(42)
42
>>> rewrap_const_reference_unsigned_short(42)
42
>>> rewrap_const_reference_long(42)
42
>>> rewrap_const_reference_unsigned_long(42)
42
>>> rewrap_const_reference_long_long(42) == 42
True
>>> rewrap_const_reference_unsigned_long_long(42) == 42
True


>>> assert abs(rewrap_const_reference_float(4.2) - 4.2) < .000001
>>> rewrap_const_reference_double(4.2) - 4.2
0.0
>>> rewrap_const_reference_long_double(4.2) - 4.2
0.0

>>> assert abs(rewrap_const_reference_complex_float(4+.2j) - (4+.2j)) < .000001
>>> assert abs(rewrap_const_reference_complex_double(4+.2j) - (4+.2j)) < .000001
>>> assert abs(rewrap_const_reference_complex_long_double(4+.2j) - (4+.2j)) < .000001

>>> rewrap_const_reference_cstring('hello, world')
'hello, world'
>>> rewrap_const_reference_string('yo, wassup?')
'yo, wassup?'

>>> rewrap_const_reference_handle(1)
1
>>> x = 'hi'
>>> assert rewrap_const_reference_handle(x) is x
>>> assert rewrap_const_reference_object(x) is x
>>> assert rewrap_reference_object(x) is x


Check that None <==> NULL

>>> rewrap_const_reference_cstring(None)

But None cannot be converted to a string object:

>>> try: rewrap_const_reference_string(None)
... except TypeError: pass
... else: print 'expected a TypeError exception'

Now check implicit conversions between floating/integer types

>>> rewrap_const_reference_float(42)
42.0

>>> rewrap_const_reference_float(42L)
42.0

>>> try: rewrap_const_reference_int(42.0)
... except TypeError: pass
... else: print 'expected a TypeError exception'

>>> rewrap_value_float(42)
42.0

>>> try: rewrap_value_int(42.0)
... except TypeError: pass
... else: print 'expected a TypeError exception'

Check that classic classes also work

>>> class FortyTwo:
...     def __int__(self):
...         return 42
...     def __float__(self):
...         return 42.0
...     def __complex__(self):
...         return complex(4+.2j)
...     def __str__(self):
...         return '42'

>>> try: rewrap_const_reference_float(FortyTwo())
... except TypeError: pass
... else: print 'expected a TypeError exception'

>>> try: rewrap_value_int(FortyTwo())
... except TypeError: pass
... else: print 'expected a TypeError exception'

>>> try: rewrap_const_reference_string(FortyTwo())
... except TypeError: pass
... else: print 'expected a TypeError exception'

>>> try: rewrap_value_complex_double(FortyTwo())
... except TypeError: pass
... else: print 'expected a TypeError exception'

# show that arbitrary handle<T> instantiations can be returned
>>> assert get_type(1) is type(1)

>>> assert return_null_handle() is None
"""

def run(args = None):
    import sys
    import doctest
    import builtin_converters_ext
    
    if 'rewrap_value_long_long' in dir(builtin_converters_ext):
        print 'LONG_LONG supported, testing...'
    else:
        print 'LONG_LONG not supported, skipping those tests...'
        
    if args is not None:
        sys.argv = args
    return doctest.testmod(sys.modules.get(__name__))
    
if __name__ == '__main__':
    print "running..."
    import sys
    status = run()[0]
    if (status == 0): print "Done."
    sys.exit(status)

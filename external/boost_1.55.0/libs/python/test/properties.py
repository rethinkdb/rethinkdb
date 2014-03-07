# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
"""
This is test module for properties.

>>> r = properties.ret_type()
>>> r.i = 22.5
>>> r.i
22.5
>>> c = properties.crash_me()
>>> c.i.i
42.5

>>> X = properties.X

>>> x1 = X(1)

value read only
>>> x1.value_r
1

value read - write
>>> x1.value_rw
1

value direct access
>>> x1.value_direct
1

class instance count read - only
>>> X.instance_count
1

class instance count direct
>>> X.instance_count_direct
1

class instance count injected
>>> X.instance_count_injected
1

class instance count from object
>>> x1.instance_count
1

class instance count from object
>>> x1.instance_count_direct
1

class instance count from object: 
>>> x1.instance_count_injected
1

as expected you can't assign new value to read only property
>>> x1.value_r = 2
Traceback (most recent call last):
  File "properties.py", line 49, in ?
    x1.value_r = 2
AttributeError: can't set attribute

setting value_rw to 2. value_direct: 
>>> x1.value_rw = 2
>>> x1.value_rw
2

setting value_direct to 3. value_direct: 
>>> x1.value_direct = 3
>>> x1.value_direct
3

>>> assert x1.value_r == 3

>>> x2 = X(2)

after creating second intstance of X instances count is 2
>>> x2.instance_count
2

>>> del x2
>>> assert x1.instance_count == 1

>>> assert properties.X.value_r_ds.__doc__ == "value_r_ds is read-only"

>>> assert properties.X.value_rw_ds.__doc__ == "value_rw_ds is read-write"

"""

#import sys; sys.path.append(r'P:\Actimize4.0\smart_const\py_smart_const___Win32_Debug')
import properties_ext as properties


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

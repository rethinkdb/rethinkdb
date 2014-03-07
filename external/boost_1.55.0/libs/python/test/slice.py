# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
"""
>>> from slice_ext import *
>>> accept_slice(slice(1, None, (1,2)))
1
>>> try:
...     accept_slice(list((1,2)))
...     print "test failed"
... except:
...     print "test passed"
...
test passed
>>> try:
...     from Numeric import array
... except:
...     print 1
... else:
...     check_numeric_array_rich_slice('Numeric', 'ArrayType', lambda x:x)
...
1
>>> try:
...     from numarray import array, all
... except:
...     print 1
... else:
...     check_numeric_array_rich_slice('numarray', 'NDArray', all)
...
1
>>> import sys
>>> if sys.version_info[0] == 2 and sys.version_info[1] >= 3:
...     check_string_rich_slice()
... elif sys.version_info[0] > 2:
...     check_string_rich_slice()
... else:
...     print 1
...
1
>>> check_slice_get_indices( slice(None))
0
>>> check_slice_get_indices( slice(2,-2))
0
>>> check_slice_get_indices( slice(2, None, 2))
5
>>> check_slice_get_indices( slice(2, None, -1))
-12
>>> check_slice_get_indices( slice( 20, None))
0
>>> check_slice_get_indices( slice( -2, -5, -2))
6
"""

# Performs an affirmative and negative argument resolution check,
# checks the operation of extended slicing in Numeric arrays
#   (only performed if Numeric.array or numarray.array can be found).
# checks the operation of extended slicing in new strings (Python 2.3 only).

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

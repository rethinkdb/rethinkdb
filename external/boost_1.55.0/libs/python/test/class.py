# Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
>>> from class_ext import *

Ensure sanity:

    >>> x = X(42)
    >>> x_function(x)
    42

Demonstrate extraction in the presence of metaclass changes:

    >>> class MetaX(X.__class__):
    ...     def __new__(cls, *args):
    ...         return super(MetaX, cls).__new__(cls, *args)
    >>> class XPlusMetatype(X):
    ...     __metaclass__ = MetaX
    >>> x = XPlusMetatype(42)
    >>> x_function(x)
    42


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

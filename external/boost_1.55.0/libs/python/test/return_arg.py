# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
>>> from return_arg_ext import *
>>> l1=Label()
>>> assert l1 is l1.label("bar")
>>> assert l1 is l1.label("bar").sensitive(0)
>>> assert l1.label("foo").sensitive(0) is l1.sensitive(1).label("bar")
>>> assert return_arg is return_arg(return_arg)

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

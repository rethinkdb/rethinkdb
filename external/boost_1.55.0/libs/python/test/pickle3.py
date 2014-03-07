# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
r'''>>> import pickle3_ext
    >>> import pickle
    >>> pickle3_ext.world.__module__
    'pickle3_ext'
    >>> pickle3_ext.world.__safe_for_unpickling__
    1
    >>> pickle3_ext.world.__getstate_manages_dict__
    1
    >>> pickle3_ext.world.__name__
    'world'
    >>> pickle3_ext.world('Hello').__reduce__()
    (<class 'pickle3_ext.world'>, ('Hello',), ({}, 0))
    >>> for number in (24, 42):
    ...   wd = pickle3_ext.world('California')
    ...   wd.set_secret_number(number)
    ...   wd.x = 2 * number
    ...   wd.y = 'y' * number
    ...   wd.z = 3. * number
    ...   pstr = pickle.dumps(wd)
    ...   wl = pickle.loads(pstr)
    ...   print wd.greet(), wd.get_secret_number(), wd.x, wd.y, wd.z
    ...   print wl.greet(), wl.get_secret_number(), wl.x, wl.y, wl.z
    Hello from California! 24 48 yyyyyyyyyyyyyyyyyyyyyyyy 72.0
    Hello from California! 24 48 yyyyyyyyyyyyyyyyyyyyyyyy 72.0
    Hello from California! 42 84 yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy 126.0
    Hello from California! 0 84 yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy 126.0
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

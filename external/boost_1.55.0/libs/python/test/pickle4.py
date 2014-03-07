# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
r'''>>> import pickle4_ext
    >>> import pickle
    >>> def world_getinitargs(self):
    ...   return (self.get_country(),)
    >>> pickle4_ext.world.__getinitargs__ = world_getinitargs
    >>> pickle4_ext.world.__module__
    'pickle4_ext'
    >>> pickle4_ext.world.__safe_for_unpickling__
    1
    >>> pickle4_ext.world.__name__
    'world'
    >>> pickle4_ext.world('Hello').__reduce__()
    (<class 'pickle4_ext.world'>, ('Hello',))
    >>> wd = pickle4_ext.world('California')
    >>> pstr = pickle.dumps(wd)
    >>> wl = pickle.loads(pstr)
    >>> print wd.greet()
    Hello from California!
    >>> print wl.greet()
    Hello from California!
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

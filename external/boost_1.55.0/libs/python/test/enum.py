# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
>>> from enum_ext import *

>>> identity(color.red) # in case of duplicated enums it always take the last enum
enum_ext.color.blood

>>> identity(color.green)
enum_ext.color.green

>>> identity(color.blue)
enum_ext.color.blue

>>> identity(color(1)) # in case of duplicated enums it always take the last enum
enum_ext.color.blood

>>> identity(color(2))
enum_ext.color.green

>>> identity(color(3))
enum_ext.color(3)

>>> identity(color(4))
enum_ext.color.blue

  --- check export to scope ---

>>> identity(red)
enum_ext.color.blood

>>> identity(green)
enum_ext.color.green

>>> identity(blue)
enum_ext.color.blue

>>> try: identity(1)
... except TypeError: pass
... else: print 'expected a TypeError'

>>> c = colorized()
>>> c.x
enum_ext.color.blood
>>> c.x = green
>>> c.x
enum_ext.color.green
>>> red == blood
True
>>> red == green
False
>>> hash(red) == hash(blood)
True
>>> hash(red) == hash(green)
False
'''

# pickling of enums only works with Python 2.3 or higher
exercise_pickling = '''
>>> import pickle
>>> p = pickle.dumps(color.green, pickle.HIGHEST_PROTOCOL)
>>> l = pickle.loads(p)
>>> identity(l)
enum_ext.color.green
'''

def run(args = None):
    import sys
    import doctest
    import pickle

    if args is not None:
        sys.argv = args
    self = sys.modules.get(__name__)
    if (hasattr(pickle, "HIGHEST_PROTOCOL")):
      self.__doc__ += exercise_pickling
    return doctest.testmod(self)

if __name__ == '__main__':
    print "running..."
    import sys
    status = run()[0]
    if (status == 0): print "Done."
    sys.exit(status)

# Copyright Joel de Guzman 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''

#####################################################################
# Check an object that we will use as container element
#####################################################################

>>> from map_indexing_suite_ext import *
>>> assert "map_indexing_suite_IntMap_entry" in dir()
>>> assert "map_indexing_suite_TestMap_entry" in dir()
>>> assert "map_indexing_suite_XMap_entry" in dir()
>>> assert "map_indexing_suite_AMap_entry" in dir()
>>> x = X('hi')
>>> x
hi
>>> x.reset() # a member function that modifies X
>>> x
reset
>>> x.foo() # another member function that modifies X
>>> x
foo

# test that a string is implicitly convertible
# to an X
>>> x_value('bochi bochi') 
'gotya bochi bochi'

#####################################################################
# Iteration
#####################################################################
>>> def print_xmap(xmap):
...     s = '[ '
...     for x in xmap: 
...         s += repr(x)
...         s += ' ' 
...     s += ']'
...     print s

#####################################################################
# Setting (adding entries)
#####################################################################
>>> xm = XMap()
>>> xm['joel'] = 'apple'
>>> xm['tenji'] = 'orange'
>>> xm['mariel'] = 'grape'
>>> xm['tutit'] = 'banana'
>>> xm['kim'] = 'kiwi'

>>> print_xmap(xm)
[ (joel, apple) (kim, kiwi) (mariel, grape) (tenji, orange) (tutit, banana) ]

#####################################################################
# Changing an entry
#####################################################################
>>> xm['joel'] = 'pineapple'
>>> print_xmap(xm)
[ (joel, pineapple) (kim, kiwi) (mariel, grape) (tenji, orange) (tutit, banana) ]

#####################################################################
# Deleting an entry
#####################################################################
>>> del xm['joel']
>>> print_xmap(xm)
[ (kim, kiwi) (mariel, grape) (tenji, orange) (tutit, banana) ]

#####################################################################
# adding an entry
#####################################################################
>>> xm['joel'] = 'apple'
>>> print_xmap(xm)
[ (joel, apple) (kim, kiwi) (mariel, grape) (tenji, orange) (tutit, banana) ]

#####################################################################
# Indexing
#####################################################################
>>> len(xm)
5
>>> xm['joel']
apple
>>> xm['tenji']
orange
>>> xm['mariel']
grape
>>> xm['tutit']
banana
>>> xm['kim']
kiwi

#####################################################################
# Calling a mutating function of a container element
#####################################################################
>>> xm['joel'].reset()
>>> xm['joel']
reset

#####################################################################
# Copying a container element
#####################################################################
>>> x = X(xm['mariel'])
>>> x
grape
>>> x.foo()
>>> x
foo
>>> xm['mariel'] # should not be changed to 'foo'
grape

#####################################################################
# Referencing a container element
#####################################################################
>>> x = xm['mariel']
>>> x
grape
>>> x.foo()
>>> x
foo
>>> xm['mariel'] # should be changed to 'foo'
foo

>>> xm['mariel'] = 'grape' # take it back
>>> xm['joel'] = 'apple' # take it back

#####################################################################
# Contains
#####################################################################
>>> assert 'joel' in xm
>>> assert 'mariel' in xm
>>> assert 'tenji' in xm
>>> assert 'tutit' in xm
>>> assert 'kim' in xm
>>> assert not 'X' in xm
>>> assert not 12345 in xm

#####################################################################
# Some references to the container elements 
#####################################################################

>>> z0 = xm['joel']
>>> z1 = xm['mariel']
>>> z2 = xm['tenji']
>>> z3 = xm['tutit']
>>> z4 = xm['kim']

>>> z0 # proxy
apple
>>> z1 # proxy
grape
>>> z2 # proxy
orange
>>> z3 # proxy
banana
>>> z4 # proxy
kiwi

#####################################################################
# Delete some container element 
#####################################################################

>>> del xm['tenji']
>>> print_xmap(xm)
[ (joel, apple) (kim, kiwi) (mariel, grape) (tutit, banana) ]

>>> del xm['tutit']
>>> print_xmap(xm)
[ (joel, apple) (kim, kiwi) (mariel, grape) ]

#####################################################################
# Show that the references are still valid 
#####################################################################
>>> z0 # proxy
apple
>>> z1 # proxy
grape
>>> z2 # proxy detached
orange
>>> z3 # proxy detached
banana
>>> z4 # proxy
kiwi

#####################################################################
# Show that iteration allows mutable access to the elements
#####################################################################
>>> for x in xm:
...     x.data().reset()
>>> print_xmap(xm)
[ (joel, reset) (kim, reset) (mariel, reset) ]

#####################################################################
# Some more...
#####################################################################

>>> tm = TestMap()
>>> tm["joel"] = X("aaa")
>>> tm["kimpo"] = X("bbb")
>>> print_xmap(tm)
[ (joel, aaa) (kimpo, bbb) ]
>>> for el in tm: #doctest: +NORMALIZE_WHITESPACE
...     print el.key(),
...     dom = el.data()
joel kimpo

#####################################################################
# Test custom converter...
#####################################################################

>>> am = AMap()
>>> am[3] = 4
>>> am[3]
4
>>> for i in am:
...     i.data()
4

#####################################################################
# END.... 
#####################################################################

'''


def run(args = None):
    import sys
    import doctest

    if args is not None:
        sys.argxm = args
    return doctest.testmod(sys.modules.get(__name__))

if __name__ == '__main__':
    print 'running...'
    import sys
    status = run()[0]
    if (status == 0): print "Done."
    sys.exit(status)






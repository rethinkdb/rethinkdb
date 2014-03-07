# Copyright Joel de Guzman 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''

#####################################################################
# Check an object that we will use as container element
#####################################################################

>>> from vector_indexing_suite_ext import *
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
>>> def print_xvec(xvec):
...     s = '[ '
...     for x in xvec: 
...         s += repr(x)
...         s += ' ' 
...     s += ']'
...     print s

#####################################################################
# Replace all the contents using slice syntax
#####################################################################

>>> v = XVec()
>>> v[:] = [X('a'),X('b'),X('c'),X('d'),X('e')]
>>> print_xvec(v)
[ a b c d e ]

#####################################################################
# Indexing
#####################################################################
>>> len(v)
5
>>> v[0]
a
>>> v[1]
b
>>> v[2]
c
>>> v[3]
d
>>> v[4]
e
>>> v[-1]
e
>>> v[-2]
d
>>> v[-3]
c
>>> v[-4]
b
>>> v[-5]
a

#####################################################################
# Deleting an element
#####################################################################

>>> del v[0]
>>> v[0] = 'yaba' # must do implicit conversion
>>> print_xvec(v)
[ yaba c d e ]

#####################################################################
# Calling a mutating function of a container element
#####################################################################
>>> v[3].reset()
>>> v[3]
reset

#####################################################################
# Copying a container element
#####################################################################
>>> x = X(v[3])
>>> x
reset
>>> x.foo()
>>> x
foo
>>> v[3] # should not be changed to 'foo'
reset

#####################################################################
# Referencing a container element
#####################################################################
>>> x = v[3]
>>> x
reset
>>> x.foo()
>>> x
foo
>>> v[3] # should be changed to 'foo'
foo

#####################################################################
# Slice
#####################################################################

>>> sl = v[0:2]
>>> print_xvec(sl)
[ yaba c ]
>>> sl[0].reset()
>>> sl[0]
reset

#####################################################################
# Reset the container again
#####################################################################
>>> v[:] = ['a','b','c','d','e'] # perform implicit conversion to X
>>> print_xvec(v)
[ a b c d e ]

#####################################################################
# Slice: replace [1:3] with an element
#####################################################################
>>> v[1:3] = X('z')
>>> print_xvec(v)
[ a z d e ]

#####################################################################
# Slice: replace [0:2] with a list
#####################################################################
>>> v[0:2] = ['1','2','3','4'] # perform implicit conversion to X
>>> print_xvec(v)
[ 1 2 3 4 d e ]

#####################################################################
# Slice: delete [3:4]
#####################################################################
>>> del v[3:4]
>>> print_xvec(v)
[ 1 2 3 d e ]

#####################################################################
# Slice: set [3:] to a list
#####################################################################
>>> v[3:] = [X('trailing'), X('stuff')] # a list
>>> print_xvec(v)
[ 1 2 3 trailing stuff ]

#####################################################################
# Slice: delete [:3]
#####################################################################
>>> del v[:3]
>>> print_xvec(v)
[ trailing stuff ]

#####################################################################
# Slice: insert a tuple to [0:0] 
#####################################################################
>>> v[0:0] = ('leading','stuff') # can also be a tuple
>>> print_xvec(v)
[ leading stuff trailing stuff ]

#####################################################################
# Reset the container again 
#####################################################################
>>> v[:] = ['a','b','c','d','e']

#####################################################################
# Some references to the container elements 
#####################################################################
>>> z0 = v[0]
>>> z1 = v[1]
>>> z2 = v[2]
>>> z3 = v[3]
>>> z4 = v[4]

>>> z0 # proxy
a
>>> z1 # proxy
b
>>> z2 # proxy
c
>>> z3 # proxy
d
>>> z4 # proxy
e

#####################################################################
# Delete a container element 
#####################################################################

>>> del v[2]
>>> print_xvec(v)
[ a b d e ]

#####################################################################
# Show that the references are still valid 
#####################################################################
>>> z0 # proxy
a
>>> z1 # proxy
b
>>> z2 # proxy detached
c
>>> z3 # proxy index adjusted
d
>>> z4 # proxy index adjusted
e

#####################################################################
# Delete all container elements
#####################################################################
>>> del v[:]
>>> print_xvec(v)
[ ]

#####################################################################
# Show that the references are still valid 
#####################################################################
>>> z0 # proxy detached
a
>>> z1 # proxy detached
b
>>> z2 # proxy detached
c
>>> z3 # proxy detached
d
>>> z4 # proxy detached
e

#####################################################################
# Reset the container again 
#####################################################################
>>> v[:] = ['a','b','c','d','e']

#####################################################################
# renew the references to the container elements 
#####################################################################
>>> z0 = v[0]
>>> z1 = v[1]
>>> z2 = v[2]
>>> z3 = v[3]
>>> z4 = v[4]

>>> z0 # proxy
a
>>> z1 # proxy
b
>>> z2 # proxy
c
>>> z3 # proxy
d
>>> z4 # proxy
e

#####################################################################
# Set [2:4] to a list such that there will be more elements 
#####################################################################
>>> v[2:4] = ['x','y','v']
>>> print_xvec(v)
[ a b x y v e ]

#####################################################################
# Show that the references are still valid 
#####################################################################
>>> z0 # proxy
a
>>> z1 # proxy
b
>>> z2 # proxy detached
c
>>> z3 # proxy detached
d
>>> z4 # proxy index adjusted
e

#####################################################################
# Contains
#####################################################################
>>> v[:] = ['a','b','c','d','e'] # reset again

>>> assert 'a' in v
>>> assert 'b' in v
>>> assert 'c' in v
>>> assert 'd' in v
>>> assert 'e' in v
>>> assert not 'X' in v
>>> assert not 12345 in v

#####################################################################
# Show that iteration allows mutable access to the elements
#####################################################################
>>> v[:] = ['a','b','c','d','e'] # reset again
>>> for x in v:
...     x.reset()
>>> print_xvec(v)
[ reset reset reset reset reset ]

#####################################################################
# append
#####################################################################
>>> v[:] = ['a','b','c','d','e'] # reset again
>>> v.append('f')
>>> print_xvec(v)
[ a b c d e f ]

#####################################################################
# extend
#####################################################################
>>> v[:] = ['a','b','c','d','e'] # reset again
>>> v.extend(['f','g','h','i','j'])
>>> print_xvec(v)
[ a b c d e f g h i j ]

#####################################################################
# extend using a generator expression
#####################################################################
>>> v[:] = ['a','b','c','d','e'] # reset again
>>> def generator():
...   addlist = ['f','g','h','i','j']
...   for i in addlist:
...     if i != 'g':
...       yield i
>>> v.extend(generator())
>>> print_xvec(v)
[ a b c d e f h i j ]

#####################################################################
# vector of strings
#####################################################################
>>> sv = StringVec()
>>> sv.append('a')
>>> print sv[0]
a

#####################################################################
# END.... 
#####################################################################

'''


def run(args = None):
    import sys
    import doctest

    if args is not None:
        sys.argv = args
    return doctest.testmod(sys.modules.get(__name__))

if __name__ == '__main__':
    print 'running...'
    import sys
    status = run()[0]
    if (status == 0): print "Done."
    sys.exit(status)






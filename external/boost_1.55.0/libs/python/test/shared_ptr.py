# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
>>> from shared_ptr_ext import *

   Test that shared_ptr<Derived> can be converted to shared_ptr<Base>
 
>>> Y.store(YYY(42))

>>> x = X(17)
>>> null_x = null(x)
>>> null_x # should be None
>>> identity(null_x) # should also be None

>>> a = New(1)
>>> A.call_f(a)
1
>>> New(0)

>>> type(factory(3))
<class 'shared_ptr_ext.Y'>
>>> type(factory(42))
<class 'shared_ptr_ext.YY'>

>>> class P(Z):
...     def v(self):
...         return -Z.v(self);
...     def __del__(self):
...         print 'bye'
...
>>> p = P(12)
>>> p.value()
12
>>> p.v()
-12
>>> look(p)
12
>>> try: modify(p)
... except TypeError: pass
... else: 'print expected a TypeError'
>>> look(None)
-1
>>> store(p)
>>> del p
>>> Z.get().v()
-12
>>> Z.count()
1
>>> Z.look_store()
12
>>> Z.release()
bye
>>> Z.count()
0

>>> z = Z(13)
>>> z.value()
13
>>> z.v()
13
>>> try: modify(z)
... except TypeError: pass
... else: 'print expected a TypeError'

>>> Z.get() # should be None
>>> store(z)
>>> assert Z.get() is z  # show that deleter introspection works
>>> del z
>>> Z.get().value()
13
>>> Z.count()
1
>>> Z.look_store()
13
>>> Z.release()
>>> Z.count()
0

>>> x = X(17)
>>> x.value()
17
>>> look(x)
17
>>> try: modify(x)
... except TypeError: pass
... else: 'print expected a TypeError'
>>> look(None)
-1
>>> store(x)
>>> del x
>>> X.count()
1
>>> X.look_store()
17
>>> X.release()
>>> X.count()
0


>>> y = Y(19)
>>> y.value()
19
>>> modify(y)
>>> look(y)
-1
>>> store(Y(23))
>>> Y.count()
1
>>> Y.look_store()
23
>>> Y.release()
>>> Y.count()
0
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

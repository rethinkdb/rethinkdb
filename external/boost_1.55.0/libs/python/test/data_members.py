# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
>>> from data_members_ext import *

        ---- Test static data members ---

>>> v = Var('slim shady')

>>> Var.ro2a.x
0
>>> Var.ro2b.x
0
>>> Var.rw2a.x
0
>>> Var.rw2b.x
0
>>> v.ro2a.x
0
>>> v.ro2b.x
0
>>> v.rw2a.x
0
>>> v.rw2b.x
0
>>> Var.rw2a.x = 777
>>> Var.ro2a.x
777
>>> Var.ro2b.x
777
>>> Var.rw2a.x
777
>>> Var.rw2b.x
777
>>> v.ro2a.x
777
>>> v.ro2b.x
777
>>> v.rw2a.x
777
>>> v.rw2b.x
777
>>> Var.rw2b = Y(888)

>>> y = Y(99)
>>> y.q = True
>>> y.q
True
>>> y.q = False
>>> y.q
False

>>> Var.ro2a.x
888
>>> Var.ro2b.x
888
>>> Var.rw2a.x
888
>>> Var.rw2b.x
888
>>> v.ro2a.x
888
>>> v.ro2b.x
888
>>> v.rw2a.x
888
>>> v.rw2b.x
888
>>> v.rw2b.x = 999
>>> Var.ro2a.x
999
>>> Var.ro2b.x
999
>>> Var.rw2a.x
999
>>> Var.rw2b.x
999
>>> v.ro2a.x
999
>>> v.ro2b.x
999
>>> v.rw2a.x
999
>>> v.rw2b.x
999


>>> Var.ro1a
0
>>> Var.ro1b
0
>>> Var.rw1a
0
>>> Var.rw1b
0
>>> v.ro1a
0
>>> v.ro1b
0
>>> v.rw1a
0
>>> v.rw1b
0
>>> Var.rw1a = 777
>>> Var.ro1a
777
>>> Var.ro1b
777
>>> Var.rw1a
777
>>> Var.rw1b
777
>>> v.ro1a
777
>>> v.ro1b
777
>>> v.rw1a
777
>>> v.rw1b
777
>>> Var.rw1b = 888
>>> Var.ro1a
888
>>> Var.ro1b
888
>>> Var.rw1a
888
>>> Var.rw1b
888
>>> v.ro1a
888
>>> v.ro1b
888
>>> v.rw1a
888
>>> v.rw1b
888
>>> v.rw1b = 999
>>> Var.ro1a
999
>>> Var.ro1b
999
>>> Var.rw1a
999
>>> Var.rw1b
999
>>> v.ro1a
999
>>> v.ro1b
999
>>> v.rw1a
999
>>> v.rw1b
999



        -----------------
        
>>> x = X(42)
>>> x.x
42
>>> try: x.x = 77
... except AttributeError: pass
... else: print 'no error'

>>> x.fair_value
42.0
>>> y = Y(69)
>>> y.x
69
>>> y.x = 77
>>> y.x
77

>>> v = Var("pi")
>>> v.value = 3.14
>>> v.name
'pi'
>>> v.name2
'pi'

>>> v.get_name1()
'pi'

>>> v.get_name2()
'pi'

>>> v.y.x
6
>>> v.y.x = -7
>>> v.y.x
-7

>>> v.name3
'pi'


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

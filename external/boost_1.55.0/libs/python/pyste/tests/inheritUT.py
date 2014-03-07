# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import unittest
from _inherit import *

class InheritExampleTest(unittest.TestCase):

    def testIt(self):
        a = A_int()        
        b = B()
        self.assert_(isinstance(b, A_int))
        self.assert_(issubclass(B, A_int))
        a.set(10)
        self.assertEqual(a.get(), 10)        
        b.set(1)
        self.assertEqual(b.go(), 1)
        self.assertEqual(b.get(), 1)

        d = D()
        self.assert_(issubclass(D, B))
        self.assertEqual(d.x, 0)
        self.assertEqual(d.y, 0)
        self.assertEqual(d.s, 1)
        self.assertEqual(D.s, 1)
        self.assertEqual(d.f1(), 1)
        self.assertEqual(d.f2(), 2)



if __name__ == '__main__':
    unittest.main() 

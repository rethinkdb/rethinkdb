# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import unittest
from _inherit4 import *

class TestInherit4(unittest.TestCase):

    def testIt(self):
        self.assert_(issubclass(B, A))
        self.assert_(issubclass(C, A))
        self.assert_(issubclass(C, B))
        a = A()
        a.x = 1
        b = B()
        b.x = 10
        b.y = 20
        c = C()
        c.x = 100
        c.y = 200
        c.z = 300
        self.assertEqual(a.x, 1)
        self.assertEqual(b.x, 10)
        self.assertEqual(b.y, 20)
        self.assertEqual(c.x, 100)
        self.assertEqual(c.y, 200)
        self.assertEqual(c.z, 300)

if __name__ == '__main__':
    unittest.main()

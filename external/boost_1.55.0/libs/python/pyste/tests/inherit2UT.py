# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import unittest
from _inherit2 import *

class InheritExampleTest(unittest.TestCase):

    def testIt(self):
        b = B()
        d = D()
        
        self.assert_(issubclass(D, B))
        b.x, b.y = 10, 5
        self.assertEqual(b.getx(), 10)
        self.assertEqual(b.gety(), 5)
        d.x, d.y, d.z, d.w = 20, 15, 10, 5
        self.assertEqual(d.getx(), 20)
        self.assertEqual(d.gety(), 15) 
        self.assertEqual(d.getz(), 10)
        self.assertEqual(d.getw(), 5) 
        self.assertEqual(b.foo(), 1)
        self.assertEqual(b.foo(3), 3)

        def wrong():
            return b.getw()
        self.assertRaises(AttributeError, wrong)

if __name__ == '__main__':
    unittest.main() 

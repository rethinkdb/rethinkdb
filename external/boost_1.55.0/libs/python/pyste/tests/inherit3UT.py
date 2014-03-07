# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import unittest
from _inherit3 import *

class testInherit3(unittest.TestCase):

    def testIt(self):
        def testInst(c):
            self.assertEqual(c.x, 0)
            self.assertEqual(c.foo(3), 3)
            x = c.X()
            self.assertEqual(x.y, 0)
            self.assertEqual(c.E.i, 0)
            self.assertEqual(c.E.j, 1)
        b = B()
        c = C()
        testInst(b)
        testInst(c)
        self.assertEqual(b.foo(), 1)
        self.assertEqual(c.foo(), 0)


if __name__ == '__main__':
    unittest.main()

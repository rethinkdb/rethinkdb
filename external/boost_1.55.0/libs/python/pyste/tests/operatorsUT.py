# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import unittest
from _operators import *

class OperatorTest(unittest.TestCase):

    def testIt(self):              
        c = C()
        c.value = 3.0
        d = C()
        d.value = 2.0
        self.assertEqual(c.x, 10)
        self.assertEqual(C.x, 10)
        self.assertEqual(C.x, 10)
        self.assertEqual((c * d).value, 6.0)
        self.assertEqual((c + d).value, 5.0)
        self.assertEqual(int(c), 3)
        self.assertEqual(int(d), 2)
        self.assertEqual(c(), 10)
        self.assertEqual(d(), 10)
        self.assertEqual(c(3.0), 13.0)
        self.assertEqual(d(6.0), 16.0)
        self.assertEqual(str(c), "C")


if __name__ == '__main__':
    unittest.main()  

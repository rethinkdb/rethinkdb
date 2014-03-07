# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import unittest
from _add_test import *

class AddMethodTest(unittest.TestCase):

    def testIt(self):
        c = C()
        c.x = 10
        self.assertEqual(c.get_x(), 10)

if __name__ == '__main__':
    unittest.main()

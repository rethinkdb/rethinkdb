# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import unittest
from _code_test import *

class CodeTest(unittest.TestCase):

    def testIt(self):
        a = A()
        a.x = 12
        self.assertEqual(get(a), 12)
        self.assertEqual(foo(), "Hello!")


if __name__ == '__main__':
    unittest.main()

# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt) 
import unittest
from _enums import *

class EnumsTest(unittest.TestCase):

    def testIt(self):
        self.assertEqual(int(Red), 0)
        self.assertEqual(int(Blue), 1)

        self.assertEqual(int(X.Choices.Good), 1)
        self.assertEqual(int(X.Choices.Bad), 2)
        a = X()
        self.assertEqual(a.set(a.Choices.Good), 1)
        self.assertEqual(a.set(a.Choices.Bad), 2)
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)


if __name__ == '__main__':
    unittest.main()

# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

import unittest
from _virtual2 import *

class Virtual2Test(unittest.TestCase):

    def testIt(self):
        a = A()
        self.assertEqual(a.f1(), 10)
        b = B()
        self.assertEqual(b.f1(), 10)
        self.assertEqual(b.f2(), 20)
        self.assertEqual(call_fs(b), 30)
        self.assertEqual(call_f(a), 0)
        self.assertEqual(call_f(b), 1)
        nb = b.make_new()
        na = a.make_new()
        self.assertEqual(na.f1(), 10)
        self.assertEqual(nb.f1(), 10)
        self.assertEqual(nb.f2(), 20)
        self.assertEqual(call_fs(nb), 30)
        self.assertEqual(call_f(na), 0)
        self.assertEqual(call_f(nb), 1) 
        class C(B):
            def f1(self): return 1
            def f2(self): return 2
            def f(self): return 100

        c = C()
        self.assertEqual(call_fs(c), 3)
        self.assertEqual(call_fs(c), 3)
        self.assertEqual(call_f(c), 100)


if __name__ == '__main__':
    unittest.main()  

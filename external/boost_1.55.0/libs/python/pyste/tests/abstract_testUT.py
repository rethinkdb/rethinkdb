# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt) 
import unittest
from _abstract_test import *

class AbstractTest(unittest.TestCase):

    def testIt(self):
        class C(A):
            def f(self):
                return 'C::f'

        a = A()
        b = B()
        c = C()
        self.assertRaises(RuntimeError, a.f)
        self.assertEqual(b.f(), 'B::f')
        self.assertEqual(call(b), 'B::f')
        self.assertEqual(c.f(), 'C::f')
        self.assertEqual(call(c), 'C::f') 


if __name__ == '__main__':
    unittest.main()

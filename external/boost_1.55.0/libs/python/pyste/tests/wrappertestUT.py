# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import unittest
from _wrappertest import *

class WrapperTest(unittest.TestCase):

    def testIt(self):              
        self.assertEqual(Range(10), range(10))
        self.assertEqual(C().Mul(10), [x*10 for x in range(10)])

        a = A()
        self.assertEqual(a.f(), 10)
        self.assertEqual(call_foo(a), 10)
        class D(A):
            def f(self): return 2
        d = D()
        self.assertEqual(d.f(), 2)
        self.assertEqual(call_foo(d), 2)

if __name__ == '__main__':
    unittest.main()  

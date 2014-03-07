# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
import unittest
from register_ptr import *

class RegisterPtrTest(unittest.TestCase):

    def testIt(self):

        class B(A):
            def f(self):
                return 10

        a = New()  # this must work
        b = B()
        self.assertEqual(Call(a), 0)                                    
        self.assertEqual(Call(b), 10)                     
        def fails():
            Fail(A())
        self.assertRaises(TypeError, fails)
        self.assertEqual(Fail(a), 0) # ok, since a is held by shared_ptr

if __name__ == '__main__':
    unittest.main()

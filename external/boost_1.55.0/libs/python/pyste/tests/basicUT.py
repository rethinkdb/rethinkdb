# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import unittest
from _basic import *

class BasicExampleTest(unittest.TestCase):

    def testIt(self):

        # test virtual functions
        class D(C):
            def f(self, x=10):
                return x+1

        d = D()
        c = C()

        self.assertEqual(c.f(), 20)
        self.assertEqual(c.f(3), 6)
        self.assertEqual(d.f(), 11)
        self.assertEqual(d.f(3), 4)
        self.assertEqual(call_f(c), 20)
        self.assertEqual(call_f(c, 4), 8)
        self.assertEqual(call_f(d), 11)
        self.assertEqual(call_f(d, 3), 4)
        
        # test data members
        def testValue(value):
            self.assertEqual(c.value, value)
            self.assertEqual(d.value, value) 
            self.assertEqual(get_value(c), value)
            self.assertEqual(get_value(d), value)
        testValue(1)
        c.value = 30
        d.value = 30
        testValue(30)
        self.assertEqual(c.const_value, 0)
        self.assertEqual(d.const_value, 0)
        def set_const_value():
            c.const_value = 12
        self.assertRaises(AttributeError, set_const_value)
        
        # test static data-members
        def testStatic(value):
            self.assertEqual(C.static_value, value)
            self.assertEqual(c.static_value, value)
            self.assertEqual(D.static_value, value)
            self.assertEqual(d.static_value, value)
            self.assertEqual(get_static(), value)
        testStatic(3)
        C.static_value = 10
        testStatic(10)
        self.assertEqual(C.const_static_value, 100)
        def set_const_static():
            C.const_static_value = 1
        self.assertRaises(AttributeError, set_const_static)

        # test static function
        def test_mul(result, *args):
            self.assertEqual(C.mul(*args), result)
            self.assertEqual(c.mul(*args), result)
        test_mul(16, 8, 2)
        test_mul(6.0, 2.0, 3.0)
        self.assertEqual(C.square(), 4)
        self.assertEqual(c.square(), 4)
        self.assertEqual(C.square(3), 9)
        self.assertEqual(c.square(3), 9)


if __name__ == '__main__':
    unittest.main()

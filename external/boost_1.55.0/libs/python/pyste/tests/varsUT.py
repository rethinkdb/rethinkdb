# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import unittest
import _vars


class VarsTest(unittest.TestCase):

    def testIt(self):
        def testColor(c, r, g, b):
            self.assertEqual(c.r, r)            
            self.assertEqual(c.g, g)            
            self.assertEqual(c.b, b)            
        testColor(_vars.black, 0, 0, 0)
        testColor(_vars.red, 255, 0, 0)
        testColor(_vars.green, 0, 255, 0)
        testColor(_vars.blue, 0, 0, 255)

if __name__ == '__main__':
    unittest.main()

# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import unittest
from _header_test import *

class HeaderTest(unittest.TestCase):
    
    def testIt(self):
        self.assertEqual(choice.red, 0)
        self.assertEqual(choice.blue, 1)
        self.assertEqual(choice_str(choice.blue), 'blue')
        self.assertEqual(choice_str(choice.red), 'red')
        c = C()
        c.c = choice.blue
        self.assertEqual(c.get(), 'blue')
        c.c = choice.red
        self.assertEqual(c.get(), 'red')
        # the following classes/functions should not have being exported
        self.assertRaises(NameError, lambda: A())
        self.assertRaises(NameError, lambda: foo())
        self.assertRaises(NameError, lambda: bar.value)
        self.assertRaises(NameError, lambda: ForwardDeclared())
        
if __name__ == '__main__':
    unittest.main()    

# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import unittest
from _virtual import *

class VirtualTest(unittest.TestCase):

    def testIt(self):              
        
        class E(C):
            def f_abs(self):
                return 3
            def dummy(self):
                # override should not work
                return 100
            
        class F(C):
            def f(self):
                return 10
            def name(self):
                return 'F'
  
        class G(D):
            def dummy(self):
                # override should not work
                return 100

        e = E()
        f = F()

        self.assertEqual(e.f(), 3)
        self.assertEqual(call_f(e), 3)
        self.assertEqual(f.f(), 10)
        self.assertEqual(call_f(f), 10)
        self.assertEqual(e.get_name(), 'C')
        #self.assertEqual(e.get_name(), 'E') check this later

        c = C()
        c.bar(1)      # ok
        c.bar('a')    # ok
        self.assertRaises(TypeError, c.bar, 1.0)        

        # test no_overrides
        d = G()        
        self.assertEqual(e.dummy(), 100)
        self.assertEqual(call_dummy(e), 0)
        self.assertEqual(d.dummy(), 100)
        self.assertEqual(call_dummy(d), 0)     


        
if __name__ == '__main__':
    unittest.main()  

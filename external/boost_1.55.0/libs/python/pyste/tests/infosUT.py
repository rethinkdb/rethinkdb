# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import sys
from Pyste.infos import *
from Pyste.policies import *
from Pyste.exporterutils import *
import unittest

#================================================================================
# InfosTest
#================================================================================
class InfosTest(unittest.TestCase):

    def testFunctionInfo(self):
        info = FunctionInfo('test::foo', 'foo.h')
        rename(info, 'hello')
        set_policy(info, return_internal_reference())
        set_wrapper(info, FunctionWrapper('foo_wrapper'))

        info = InfoWrapper(info)
        
        self.assertEqual(info.rename, 'hello')
        self.assertEqual(info.policy.Code(), 'return_internal_reference< 1 >')
        self.assertEqual(info.wrapper.name, 'foo_wrapper')


    def testClassInfo(self):
        info = ClassInfo('test::IFoo', 'foo.h')
        rename(info.name, 'Name')
        rename(info.exclude, 'Exclude')
        rename(info, 'Foo')
        rename(info.Bar, 'bar')
        set_policy(info.Baz, return_internal_reference())
        rename(info.operator['>>'], 'from_string')
        exclude(info.Bar)
        set_wrapper(info.Baz, FunctionWrapper('baz_wrapper'))
        
        info = InfoWrapper(info)
        
        self.assertEqual(info.rename, 'Foo')
        self.assertEqual(info['Bar'].rename, 'bar')
        self.assertEqual(info['name'].rename, 'Name')
        self.assertEqual(info['exclude'].rename, 'Exclude')
        self.assertEqual(info['Bar'].exclude, True)
        self.assertEqual(info['Baz'].policy.Code(), 'return_internal_reference< 1 >')
        self.assertEqual(info['Baz'].wrapper.name, 'baz_wrapper')
        self.assertEqual(info['operator']['>>'].rename, 'from_string')

     


if __name__ == '__main__':
    unittest.main()

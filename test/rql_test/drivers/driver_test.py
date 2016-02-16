#!/usr/bin/env python
# -*- coding: utf-8 -*-

import unittest
from driver import bag, compare, err, err_regex, partial, uuid

try:
    unicode
except NameError:
    unicode = str

class PythonTestDriverTest(unittest.TestCase):
    
    def compare(self, expected, result, options=None):
        self.assertTrue(compare(expected, result, options=options))
    
    def compareFalse(self, expected, result, options=None):
        self.assertFalse(compare(expected, result, options=options))
    
    def test_string(self):
        
        # simple
        self.compare('a', 'a')
        self.compare('a', unicode('a'))
        self.compare('á', 'á')
        self.compare('something longer\nwith two lines', 'something longer\nwith two lines')
        
        self.compareFalse('a', 'b')
        self.compareFalse('a', 1)
        self.compareFalse('a', [])
        self.compareFalse('a', None)
        self.compareFalse('a', ['a'])
        self.compareFalse('a', {'a':1})
    
    def test_array(self):
        
        # simple pass
        self.compare([1,2,3], [1,2,3])
        
        # out of order
        self.compareFalse([1,2,3], [1,3,2])
        
        # totally mistmatched lists
        self.compareFalse([1,2,3], [3,4,5])
        
        # missing items
        self.compareFalse([1,2,3], [1,2])
        self.compareFalse([1,2,3], [1,3])
        
        # extra items
        self.compareFalse([1,2,3], [1,2,3,4])
        
        # empty array
        self.compare([], [])
        self.compareFalse([1], [])
        self.compareFalse([], [1])
        self.compareFalse([], None)
        
        # strings
        self.compare(['a','b'], ['a','b'])
        self.compareFalse(['a','c'], ['a','b'])
        
        # multiple of a single value
        self.compare([1,2,2,3,3,3], [1,2,2,3,3,3])
        self.compareFalse([1,2,2,3,3,3], [1,2,3])
        self.compareFalse([1,2,3], [1,2,2,3,3,3])
    
    def test_array_partial(self):
        '''note that these are all in-order'''
        
        # simple
        self.compare(partial([1]), [1,2,3])
        self.compare(partial([2]), [1,2,3])
        self.compare(partial([3]), [1,2,3])
        
        self.compare(partial([1,2]), [1,2,3])
        self.compare(partial([1,3]), [1,2,3])
        
        self.compare(partial([1,2,3]), [1,2,3])
        
        self.compareFalse(partial([4]), [1,2,3])
        
        # ordered
        self.compareFalse(partial([3,2,1], ordered=True), [1,2,3])
        self.compareFalse(partial([1,3,2], ordered=True), [1,2,3])
        
        # empty array
        self.compare(partial([]), [1,2,3])
        
        # multiple of a single items
        self.compare(partial([1,2,2]), [1,2,2,3,3,3])
        self.compareFalse(partial([1,2,2,2]), [1,2,2,3,3,3])
    
    def test_array_unordered(self):
        
        # simple
        self.compare(bag([1,2]), [1,2])
        self.compare(bag([2,1]), [1,2])
        
        self.compareFalse(bag([1,2]), [1,2,3])
        self.compareFalse(bag([1,3]), [1,2,3])
        self.compareFalse(bag([3,1]), [1,2,3])
        
        # empty array
        self.compare(bag([]), [])
    
    def test_dict(self):
        
        # simple
        self.compare({'a':1, 'b':2, 'c':3}, {'a':1, 'b':2, 'c':3})
        self.compare({'a':1, 'b':2, 'c':3}, {'c':3, 'a':1, 'b':2})
        
        self.compareFalse({'a':1, 'b':2, 'c':3}, {'a':1})
        self.compareFalse({'a':1}, {'a':1, 'b':2, 'c':3})
        
        # empty
        self.compare({}, {})
        self.compareFalse({}, {'a':1})
        self.compareFalse({'a':1}, {})
    
    def test_dict_partial(self):
        
        # simple
        self.compare(partial({'a':1}), {'a':1})
        self.compare(partial({'a':1}), {'a':1, 'b':2})
        
        self.compareFalse(partial({'a':2}), {'a':1, 'b':2})
        self.compareFalse(partial({'c':1}), {'a':1, 'b':2})
        self.compareFalse(partial({'a':1, 'b':2}), {'b':2})
        
        # empty
        self.compare(partial({}), {})
        self.compare(partial({}), {'a':1})
        self.compareFalse(partial({'a':1}), {})
    
    def test_compare_dict_in_array(self):
        
        # simple
        self.compare([{'a':1}], [{'a':1}])
        self.compare([{'a':1, 'b':2}], [{'a':1, 'b':2}])
        self.compare([{'a':1}, {'b':2}], [{'a':1}, {'b':2}])
        
        self.compareFalse([{'a':1}], [{'a':1, 'b':2}])
        self.compareFalse([{'a':2, 'b':2}], [{'a':1, 'b':2}])
        self.compareFalse([{'a':2, 'c':3}], [{'a':1, 'b':2}])
        self.compareFalse([{'a':2, 'c':3}], [{'a':1}])
        self.compareFalse([{'a':1}, {'b':2}], [{'a':1, 'b':2}])
        
        # order
        self.compareFalse([{'a':1}, {'b':2}], [{'b':2}, {'a':1}])
        
        # partial
        self.compare(partial([{}]), [{'a':1, 'b':2}])
        self.compare(partial([{}]), [{'a':1, 'b':2}])
        self.compare(partial([{'a':1}]), [{'a':1, 'b':2}])
        self.compare(partial([{'a':1, 'b':2}]), [{'a':1, 'b':2}])
        self.compare(partial([{'a':1}, {'b':2}]), [{'a':1}, {'b':2}, {'c':3}])
        
        self.compareFalse(partial([{'a':2}]), [{'a':1, 'b':2}])
        self.compareFalse(partial([{'a':1, 'b':2}]), [{'a':1}])
        
        # partial order
        self.compareFalse(partial([{'a':1}, {'b':2}], ordered=True), [{'b':2}, {'a':1}])
        
        # partial unordered
        self.compare(partial([{'a':1}, {'b':2}]), [{'b':2}, {'a':1}])
        self.compare(partial([{'a':1}, {'b':2}], ordered=False), [{'b':2}, {'a':1}])
    
    def test_compare_partial_items_in_array(self):
        self.compare([{'a':1, 'b':1}, partial({'a':2})], [{'a':1, 'b':1}, {'a':2, 'b':2}])
    
    def test_compare_array_in_dict(self):
        pass
    
    def test_exception(self):
        
        # class only
        self.compare(KeyError, KeyError())
        self.compare(KeyError(), KeyError())        
        self.compare(err('KeyError'), KeyError())
        self.compare(err(KeyError), KeyError())
        
        self.compareFalse(KeyError, NameError())
        self.compareFalse(KeyError(), NameError())        
        self.compareFalse(err('KeyError'), NameError())
        self.compareFalse(err(KeyError), NameError())
        
        # subclass
        self.compare(LookupError, KeyError())
        self.compare(LookupError(), KeyError())        
        self.compare(err('LookupError'), KeyError())
        self.compare(err(LookupError), KeyError())
        
        self.compareFalse(KeyError, LookupError())
        self.compareFalse(KeyError(), LookupError())        
        self.compareFalse(err('KeyError'), LookupError())
        self.compareFalse(err(KeyError), LookupError())
        
        # message
        self.compare(err(KeyError), KeyError('alpha'))
        self.compare(err(KeyError, 'alpha'), KeyError('alpha'))
        
        self.compareFalse(err(KeyError, 'alpha'), KeyError('beta'))
        
        # regex message
        self.compare(err(KeyError), KeyError('alpha'))
        
        # regex message with debug/assertion text
        self.compare(err_regex(KeyError, 'alpha'), KeyError('alpha'))
        self.compare(err_regex(KeyError, 'alp'), KeyError('alpha'))
        self.compare(err_regex(KeyError, '.*pha'), KeyError('alpha'))
        
        self.compareFalse(err_regex(KeyError, 'beta'), KeyError('alpha'))
        
        # ToDo: frames (when/if we support them)
    
    def test_compare_uuid(self):
        
        # simple
        self.compare(uuid(), '4e9e5bc2-9b11-4143-9aa1-75c10e7a193a')
        self.compareFalse(uuid(), '4')
        self.compareFalse(uuid(), '*')
        self.compareFalse(uuid(), None)
    
    def test_numbers(self):
        
        # simple
        self.compare(1, 1)
        self.compare(1, 1.0)
        self.compare(1.0, 1)
        self.compare(1.0, 1.0)
        
        self.compareFalse(1, 2)
        self.compareFalse(1, 2.0)
        self.compareFalse(1.0, 2)
        self.compareFalse(1.0, 2.0)
        
        # precision
        precision = {'precision':0.5}
        self.compare(1, 1.4, precision)
        self.compare(1.0, 1.4, precision)
        
        self.compareFalse(1, 2, precision)
        self.compareFalse(1, 1.6, precision)
        self.compareFalse(1.0, 2, precision)
        self.compareFalse(1.0, 1.6, precision)

if __name__ == '__main__':
    unittest.main()

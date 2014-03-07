# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
import sys
sys.path.append('../src')
from SmartFile import *
import unittest
import tempfile
import os
import time


class SmartFileTest(unittest.TestCase):

    FILENAME = tempfile.mktemp()
    
    def setUp(self):
        self._Clean()
        
    def tearDown(self):
        self._Clean()

    def _Clean(self):
        try:
            os.remove(self.FILENAME)
        except OSError: pass

        
    def testNonExistant(self):
        "Must override the file, as there's no file in the disk yet"
        self.assert_(not os.path.isfile(self.FILENAME))
        f = SmartFile(self.FILENAME, 'w')
        f.write('Testing 123\nTesting again.')
        f.close()
        self.assert_(os.path.isfile(self.FILENAME))


    def testOverride(self):
        "Must override the file, because the contents are different"
        contents = 'Contents!\nContents!'
        # create the file normally first
        f = file(self.FILENAME, 'w')
        f.write(contents)
        f.close()
        file_time = os.path.getmtime(self.FILENAME)
        self.assert_(os.path.isfile(self.FILENAME))
        time.sleep(2)
        f = SmartFile(self.FILENAME, 'w')
        f.write(contents + '_')
        f.close()
        new_file_time = os.path.getmtime(self.FILENAME)
        self.assert_(new_file_time != file_time)


    def testNoOverride(self):
        "Must not override the file, because the contents are the same"
        contents = 'Contents!\nContents!'
        # create the file normally first
        f = file(self.FILENAME, 'w')
        f.write(contents)
        f.close()
        file_time = os.path.getmtime(self.FILENAME)
        self.assert_(os.path.isfile(self.FILENAME))
        time.sleep(2)
        f = SmartFile(self.FILENAME, 'w')
        f.write(contents)
        f.close()
        new_file_time = os.path.getmtime(self.FILENAME)
        self.assert_(new_file_time == file_time) 


    def testAutoClose(self):
        "Must be closed when garbage-collected"
        def foo():
            f = SmartFile(self.FILENAME)
            f.write('testing')  
            self.assert_(not os.path.isfile(self.FILENAME))
        foo()
        self.assert_(os.path.isfile(self.FILENAME))
        

if __name__ == '__main__':
    unittest.main()

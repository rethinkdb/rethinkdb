# Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
# distribution is subject to the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)
#!/usr/bin/python

import sys
sys.path.append('../src/Pyste')  
import unittest
import os.path
from glob import glob

if __name__ == '__main__':
    loader = unittest.defaultTestLoader
    tests = []
    for name in glob('*UT.py'):
        module = __import__(os.path.splitext(name)[0])
        tests.append(loader.loadTestsFromModule(module))
    runner = unittest.TextTestRunner()
    result = runner.run(unittest.TestSuite(tests))
    sys.exit(not result.wasSuccessful())    

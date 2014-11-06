#!/usr/bin/env python

'''Tests the driver cursor API'''

from __future__ import print_function

import os, sys, unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir, 'common')))
import utils

r = utils.import_python_driver()

port = int(sys.argv[1])
num_rows = int(sys.argv[2])

class TestCursor(unittest.TestCase):

    def setUp(self):
        c = r.connect(port=port)
        tbl = r.table('test')
        self.cur = tbl.run(c)

    def test_type(self):
        self.assertEqual(type(self.cur), r.Cursor)

    def test_count(self):
        i = 0
        for row in self.cur:
            i += 1

        self.assertEqual(i, num_rows)

    def test_close(self):
        # This excercises a code path at the root of #650
        self.cur.close()

if __name__ == '__main__':
    print("Testing cursor for %d rows" % num_rows)
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()
    suite.addTest(loader.loadTestsFromTestCase(TestCursor))
    res = unittest.TextTestRunner(verbosity=2).run(suite)

    if not res.wasSuccessful():
        sys.exit(1)

#!/usr/bin/env python

'''Tests the driver cursor API'''

from __future__ import print_function

import os, re, sys, unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir, 'common')))
import utils

r = utils.import_python_driver()

port = int(sys.argv[1])
num_rows = int(sys.argv[2])

class TestRangeCursor(unittest.TestCase):
    def setUp(self):
        self.conn = r.connect(port=port)

        if not hasattr(self, 'assertRaisesRegexp'):
            def assertRaisesRegexp_replacement(exception, regexp, function, *args, **kwds):
                try:
                    function(*args, **kwds)
                except Exception as e:
                    if not isinstance(e, Exception):
                        raise
                    if not re.match(regexp, str(e)):
                        raise AssertionError('"%s" does not match "%s"' % (str(regexp), str(e)))
                    return
                raise AssertionError('TypeError not raised for: %s' % str(function))
            self.assertRaisesRegexp = assertRaisesRegexp_replacement

    def test_cursor_after_connection_close(self):
        cursor = r.range().run(self.conn)
        self.conn.close()
        def read_cursor(cursor):
            count = 0
            for i in cursor:
                count += 1
            return count
        count = self.assertRaisesRegexp(r.RqlRuntimeError, "Connection is closed.", read_cursor, cursor)
        self.assertNotEqual(count, 0, "Did not get any cursor results")

    def test_cursor_after_cursor_close(self):
        cursor = r.range().run(self.conn)
        cursor.close()
        count = 0
        for i in cursor:
            count += 1
        self.assertNotEqual(count, 0, "Did not get any cursor results")

    def test_cursor_close_in_each(self):
        cursor = r.range().run(self.conn)
        count = 0
        for i in cursor:
            count += 1
            if count == 2:
                cursor.close()
        self.assertTrue(count > 2, "Did not get enough cursor results")

    def test_cursor_success(self):
        range_size = 10000
        cursor = r.range().limit(range_size).run(self.conn)
        count = 0
        for i in cursor:
            count += 1
        self.assertEqual(count, range_size, "Expected %d results on the cursor, but got %d" % (range_size, count))

    def test_cursor_double_each(self):
        range_size = 10000
        cursor = r.range().limit(range_size).run(self.conn)
        count = 0
        for i in cursor:
            count += 1
        self.assertEqual(count, range_size, "Expected %d results on the cursor, but got %d" % (range_size, count))
        for i in cursor:
            count += 1
        self.assertEqual(count, range_size, "Expected no results on the second iteration of the cursor, but got %d" % (count - range_size))

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
    suite.addTest(loader.loadTestsFromTestCase(TestRangeCursor))
    res = unittest.TextTestRunner(verbosity=2).run(suite)

    if not res.wasSuccessful():
        sys.exit(1)

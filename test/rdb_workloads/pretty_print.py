#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
# coding=utf8

# Environment variables:
# HOST: location of server (default = "localhost")
# PORT: port that server listens for RDB protocol traffic on (default = 14356)
# DB_NAME: database to look for test table in (default = "Welcome-db")
# TABLE_NAME: table to run tests against (default = "Welcome-rdb")

import os
import sys
import unittest
import sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import utils
r = utils.import_python_driver()

class PrettyPrintTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.conn = r.connect(
            os.environ.get('HOST', 'localhost'),
            int(os.environ.get('PORT', 28015 + 2010))
        )
        cls.table = table(os.environ.get('DB_NAME', 'Welcome-db') + "." + os.environ.get('TABLE_NAME', 'Welcome-rdb'))

    def test_pretty_print(self):
        self.assertEqual(str(r.expr(44)), "expr(44)")
        self.assertEqual(str(r.expr([expr(2)])), "expr([2])")

        # Make sure this doesn't crash
        str(r.expr([1, 2, 3]).array_to_stream().map(fn("x", R("$x") * 2)))

    def test_backtraces(self):
        try:
            self.conn.run(r.expr({"a": 1})["floop"])
        except ExecutionError as e:
            self.assertIn("floop", e.location())
        else:
            raise ValueError("that was supposed to fail")

if __name__ == "__main__":
    unittest.main()

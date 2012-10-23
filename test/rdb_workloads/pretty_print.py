#!/usr/bin/python
# coding=utf8

# Environment variables:
# HOST: location of server (default = "localhost")
# PORT: port that server listens for RDB protocol traffic on (default = 14356)
# DB_NAME: database to look for test table in (default = "Welcome-db")
# TABLE_NAME: table to run tests against (default = "Welcome-rdb")

import json
import os
import sys
import unittest
import sys
import traceback

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'drivers', 'python2')))

from rethinkdb import *
import rethinkdb.internal

class PrettyPrintTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.conn = connect(
            os.environ.get('HOST', 'localhost'),
            int(os.environ.get('PORT', 28015+2010))
            )
        cls.table = table(os.environ.get('DB_NAME', 'Welcome-db') + "." + os.environ.get('TABLE_NAME', 'Welcome-rdb'))

    def test_pretty_print(self):
        self.assertEqual(str(expr(44)), "expr(44)")
        self.assertEqual(str(expr([expr(2)])), "expr([2])")

        # Make sure this doesn't crash
        str(expr([1,2,3]).to_stream().map(fn("x", R("$x") * 2)))

    def test_backtraces(self):
        try:
            self.conn.run(expr({"a": 1})["floop"])
        except ExecutionError, e:
            self.assertIn("floop", e.location())
        else:
            raise ValueError("that was supposed to fail")

if __name__ == "__main__":
    unittest.main()

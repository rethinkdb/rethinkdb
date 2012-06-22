import rethinkdb as r
import unittest


class TestTableRef(unittest.TestCase):
    # Shared db ref, ast
    def setUp(self):
        self.conn = r.Connection("localhost", 4000)
        self.table = r.DB("testdb").testtable

    def test_table_insert(self):
        q = self.table.insert({"a": "b"}, {"c": "d"})
        resp = self.conn.run(q)
        assertEqual(resp.statuscode, 201)

        resp2 = self.conn.run(self.table)

        assertEqual(resp2.statuscode, 200)
        assertEqual(len(resp2.response), 2)

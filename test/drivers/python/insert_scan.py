
# TO RUN: python -m unittest insert_scan

import json
import os
import sys
import unittest

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'drivers', 'python')))

import rethinkdb as r

class TestTableRef(unittest.TestCase):
    # Shared db ref, ast
    def setUp(self):
        self.conn = r.Connection("localhost", 12346 + 2010)
        self.table = r.db("").Welcome
        self.docs = [{"a": 3, "b": 10, "id": 1}, {"a": 9, "b": -5, "id": 2}]

    def expect(self, query, expected):
        res = self.conn.run(query)
        self.assertEqual(res.status_code, 0, res.response[0])
        self.assertEqual(json.loads(res.response[0]), expected)

    def test_arith(self):
        expect = self.expect
        expect(r.add(3, 4), 7)
        expect(r.add(3, 4, 5), 12)

        expect(r.sub(3), -3)
        expect(r.sub(0, 3), -3)
        expect(r.sub(10, 1, 2), 7)

        expect(r.mul(1), 1)
        expect(r.mul(4, 5), 20)
        expect(r.mul(4, 5, 6), 120)

        expect(r.div(4), 1/4.)
        expect(r.div(6, 3), 2)
        expect(r.div(12, 3, 4), 1)

        expect(r.mod(4, 3), 1)

        expect(r.mul(r.add(3, 4), r.sub(6), r.add(-5, 3)), 84)

    def test_cmp(self):
        expect = self.expect

        expect(r.eq(3, 3), True)
        expect(r.eq(3, 4), False)

        expect(r.neq(3, 3), False)
        expect(r.neq(3, 4), True)

        expect(r.gt(3, 2), True)
        expect(r.gt(3, 3), False)

        expect(r.gte(3, 3), True)
        expect(r.gte(3, 4), False)

        expect(r.lt(3, 3), False)
        expect(r.lt(3, 4), True)

        expect(r.lte(3, 3), True)
        expect(r.lte(3, 4), True)
        expect(r.lte(3, 2), False)

        expect(r.eq(1, 1, 2), False)
        expect(r.eq(5, 5, 5), True)

        expect(r.lt(3, 4), True)
        expect(r.lt(3, 4, 5), True)
        expect(r.lt(5, 6, 7, 7), False)

        expect(r.eq(r.val("asdf"), r.val("asdf")), True)
        expect(r.eq(r.val("asd"), r.val("asdf")), False)

        expect(r.eq(True, True), True)
        expect(r.lt(False, True), True)

    def test_junctions(self):
        self.expect(r.any(False), False)
        self.expect(r.any(True, False), True)
        self.expect(r.any(False, True), True)
        self.expect(r.any(False, False, True), True)

        self.expect(r.all(False), False)
        self.expect(r.all(True, False), False)
        self.expect(r.all(True, True), True)
        self.expect(r.all(True, True, True), True)

    def test_let(self):
        self.expect(r.let(("x", 3), "x"), 3)
        self.expect(r.let(("x", 3), ("x", 4), "x"), 4)
        self.expect(r.let(("x", 3), ("y", 4), "x"), 3)

    def test_if(self):
        self.expect(r.if_(True, 3, 4), 3)
        self.expect(r.if_(False, 4, 5), 5)
        self.expect(r.if_(r.eq(3, 3), r.val("foo"), r.val("bar")), "foo")

    def test_attr(self):
        #self.expect(r.get())
        return

        self.expect(r.has("foo", {"foo": 3}), True)
        self.expect(r.has("bar", {"foo": 3}), False)

    def test_extend(self):
        self.expect(r.extend({"a": 5}, {"b": 3}), {"a": 5, "b": 3})
        self.expect(r.extend({"a": 5}, {"a": 3}), {"a": 3})
        self.expect(r.extend({"a": 5, "b": 1}, {"a": 3}, {"b": 6}), {"a": 3, "b": 6})

    def test_array(self):
        expect = self.expect

        expect(r.append([], 2), [2])
        expect(r.append([1], 2), [1, 2])

        expect(r.concat([1], [2]), [1, 2])
        expect(r.concat([1, 2], []), [1, 2])

        arr = range(10)
        expect(r.slice(arr, 0, 3), arr[0: 3])
        expect(r.slice(arr, 0, 0), arr[0: 0])
        expect(r.slice(arr, 5, 15), arr[5: 15])
        expect(r.slice(arr, 5, -3), arr[5: -3])
        expect(r.slice(arr, -5, -3), arr[-5: -3])

        expect(r.nth(arr, 3), 3)
        expect(r.nth(arr, -1), 9)

    def test_table_insert(self):
        q = self.table.insert(self.docs)
        resp = self.conn.run(q)
        assert resp.status_code == 0

        for doc in self.docs:
            q = self.table.find(doc['id'])
            resp = self.conn.run(q)
            assert resp.status_code == 0
            assert json.loads(resp.response[0]) == doc

if __name__ == '__main__':
    unittest.main()

#!/usr/bin/python
# coding=utf8
# TO RUN: PORT=1234 python insert_scan.py
# also respects HOST env variable (defaults to localhost)

import json
import os
import sys
import unittest
import sys
import traceback

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'drivers', 'python')))

import rethinkdb as r

class TestTableRef(unittest.TestCase):
    # Shared db ref, ast
    def setUp(self):
        self.conn = r.Connection(os.getenv('HOST') or 'localhost',
                                 12346 + (int(os.getenv('PORT') or 2010)))
        self.table = r.db("").Welcome

    def expect(self, query, *expected):
        res = self.conn.run(query)
        try:
            self.assertNotEqual(str(query), '')
            self.assertEqual(res, list(expected))
        except Exception, e:
            root_ast = r.p.Query()
            r.toTerm(query)._finalize_query(root_ast)
            print root_ast
            print res
            raise

    def expect_bad_query(self, query, msg):
        with self.assertRaises(r.BadQueryError) as cm:
            res = self.conn.run(query)
        e = cm.exception
        self.assertIn(msg, e.message)

    def expect_execution_error(self, query, msg):
        with self.assertRaises(r.ExecutionError) as cm:
            res = self.conn.run(query)
        e = cm.exception
        print "\n\n"
        print e
        self.assertIn(msg, e.message)

    def test_arith(self):
        expect = self.expect
        fail = self.expect_execution_error

        expect(r.add(3, 4), 7)
        expect(r.add(3, 4, 5), 12)
        fail(r.add(4, [0]), "number")

        expect(r.sub(3), -3)
        expect(r.sub(0, 3), -3)
        expect(r.sub(10, 1, 2), 7)
        fail(r.sub([0]), "number")
        fail(r.sub(4, [0]), "number")

        expect(r.mul(1), 1)
        expect(r.mul(4, 5), 20)
        expect(r.mul(4, 5, 6), 120)
        fail(r.mul(4, [0]), "number")

        expect(r.div(4), 1/4.)
        expect(r.div(6, 3), 2)
        expect(r.div(12, 3, 4), 1)
        fail(r.div([0]), "number")
        fail(r.div(4, [0]), "number")

        expect(r.mod(4, 3), 1)
        fail(r.mod([], 3), "number")
        fail(r.mod(3, []), "number")

        expect(r.mul(r.add(3, 4), r.sub(6), r.add(-5, 3)), 84)

    def test_cmp(self):
        expect = self.expect
        fail = self.expect_execution_error

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

        expect(r.eq("asdf", "asdf"), True)
        expect(r.eq("asd", "asdf"), False)

        expect(r.eq(True, True), True)
        expect(r.lt(False, True), True)

        expect(r.lt(False, True, 1, ""), True)
        expect(r.gt("", 1, True, False), True)
        expect(r.lt(False, True, "", 1), False)

    def test_junctions(self):
        self.expect(r.any(False), False)
        self.expect(r.any(True, False), True)
        self.expect(r.any(False, True), True)
        self.expect(r.any(False, False, True), True)

        self.expect(r.all(False), False)
        self.expect(r.all(True, False), False)
        self.expect(r.all(True, True), True)
        self.expect(r.all(True, True, True), True)

        self.expect_execution_error(r.all(True, 3), "bool")
        self.expect_execution_error(r.any(True, 4), "bool")

    def test_not(self):
        self.expect(r.not_(True), False)
        self.expect(r.not_(False), True)
        self.expect_execution_error(r.not_(3), "bool")

    def test_let(self):
        self.expect(r.let(("x", 3), r.var("x")), 3)
        self.expect(r.let(("x", 3), ("x", 4), r.var("x")), 4)
        self.expect(r.let(("x", 3), ("y", 4), r.var("x")), 3)

        self.expect_bad_query(r.var("x"), "not in scope")

    def test_if(self):
        self.expect(r.if_(True, 3, 4), 3)
        self.expect(r.if_(False, 4, 5), 5)
        self.expect(r.if_(r.eq(3, 3), "foo", "bar"), "foo")

        self.expect_execution_error(r.if_(5, 1, 2), "bool")

    def test_attr(self):
        self.expect(r.has({"foo": 3}, "foo"), True)
        self.expect(r.has({"foo": 3}, "bar"), False)

        self.expect(r.attr({"foo": 3}, "foo"), 3)
        self.expect_execution_error(r.attr({"foo": 3}, "bar"), "missing")

        self.expect(r.attr({"a": {"b": 3}}, "a.b"), 3)

    def test_extend(self):
        self.expect(r.extend({"a": 5}, {"b": 3}), {"a": 5, "b": 3})
        self.expect(r.extend({"a": 5}, {"a": 3}), {"a": 3})
        self.expect(r.extend({"a": 5, "b": 1}, {"a": 3}, {"b": 6}), {"a": 3, "b": 6})

        self.expect_execution_error(r.extend(5, {"a": 3}), "object")
        self.expect_execution_error(r.extend({"a": 5}, 5), "object")

    def test_array(self):
        expect = self.expect
        fail = self.expect_execution_error

        expect(r.append([], 2), [2])
        expect(r.append([1], 2), [1, 2])
        fail(r.append(3, 0), "array")

        expect(r.concat([1], [2]), [1, 2])
        expect(r.concat([1, 2], []), [1, 2])
        fail(r.concat(1, [1]), "array")
        fail(r.concat([1], 1), "array")

        arr = range(10)
        expect(r.slice(arr, 0, 3), arr[0: 3])
        expect(r.slice(arr, 0, 0), arr[0: 0])
        expect(r.slice(arr, 5, 15), arr[5: 15])
        expect(r.slice(arr, 5, -3), arr[5: -3])
        expect(r.slice(arr, -5, -3), arr[-5: -3])
        fail(r.slice(1, 0, 0), "array")
        fail(r.slice(arr, .5, 0), "integer")
        fail(r.slice(arr, 0, 1.01), "integer")
        fail(r.slice(arr, 5, 3), "greater")

        expect(r.element(arr, 3), 3)
        expect(r.element(arr, -1), 9)
        fail(r.element(0, 0), "array")
        fail(r.element(arr, .1), "integer")
        fail(r.element([0], 1), "bounds")

        expect(r.size([]), 0)
        expect(r.size(arr), len(arr))
        fail(r.size(0), "array")

    def test_stream(self):
        expect = self.expect
        fail = self.expect_execution_error
        arr = range(10)

        expect(r.array(r.stream(arr)), arr)
        expect(r.array(r.stream(r.array(r.stream(arr)))), arr)

        expect(r.nth(r.stream(arr), 0), 0)
        expect(r.nth(r.stream(arr), 5), 5)
        fail(r.nth(r.stream(arr), []), "integer")
        fail(r.nth(r.stream(arr), .4), "integer")
        fail(r.nth(r.stream(arr), -5), "nonnegative")
        fail(r.nth(r.stream([0]), 1), "bounds")

    def test_stream_fancy(self):
        expect = self.expect
        fail = self.expect_execution_error

        def limit(arr, count):
            return r.array(r.stream(arr).limit(count))

        expect(limit([], 0), [])
        expect(limit([1, 2], 0), [])
        expect(limit([1, 2], 1), [1])
        expect(limit([1, 2], 5), [1, 2])
        fail(limit([], -1), "nonnegative")

        # TODO: fix distinct
        return

        def distinct(arr):
            return r.array(r.stream(arr).distinct())

        expect(distinct([]), [])
        expect(distinct(range(10)*10), range(10))
        expect(distinct([1, 2, 3, 2]), [1, 2, 3])
        expect(distinct([True, 2, False, 2]), [True, 2, False])

    def test_ordering(self):
        expect = self.expect
        fail = self.expect_execution_error

        def order(arr, **kwargs):
            return r.array(r.stream(arr).orderby(**kwargs))

        arr = [{"a": n, "b": n % 3} for n in range(10)]

        from operator import itemgetter as get

        expect(order(arr, a=True), sorted(arr, key=get('a')))
        expect(order(arr, a=False), sorted(arr, key=get('a'), reverse=True))

    def clear_table(self):
        self.conn.run(self.table.delete())

    def test_table_insert(self):
        self.clear_table()

        docs = [{"a": 3, "b": 10, "id": 1}, {"a": 9, "b": -5, "id": 2}]

        self.expect(self.table.insert(docs), {'inserted': len(docs)})

        for doc in docs:
            self.expect(self.table.find(doc['id']), doc)

        self.expect(r.array(self.table.distinct('a')), [3, 9])

        self.expect(self.table.filter({"a": 3}), docs[0])

        self.expect_execution_error(self.table.filter({"a": r.add(self.table.count(), "")}), "numbers")

        self.expect_execution_error(self.table.insert({"a": 3}), "id")

    def test_unicode(self):
        self.clear_table()

        doc = {"id": 100, "text": u"グルメ"}

        self.expect(self.table.insert(doc), {'inserted': 1})
        self.expect(self.table.find(100), doc)

if __name__ == '__main__':
    unittest.main()

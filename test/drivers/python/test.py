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

from rethinkdb.abbrev import *

class RDBTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.conn = r.Connection(os.getenv('HOST') or 'localhost',
                                 12346 + (int(os.getenv('PORT') or 2010)))
        cls.table = r.db("").Welcome

    def expect(self, query, expected):
        try:
            res = self.conn.run(query)
            self.assertNotEqual(str(query), '')
            self.assertEqual(res, expected)
        except Exception, e:
            root_ast = r.p.Query()
            r.toTerm(query)._finalize_query(root_ast)
            print root_ast
            raise

    def error_query(self, query, msg):
        with self.assertRaises(r.BadQueryError) as cm:
            res = self.conn.run(query)
        e = cm.exception
        self.assertIn(msg, str(e))

    def error_exec(self, query, msg):
        with self.assertRaises(r.ExecutionError) as cm:
            res = self.conn.run(query)
        e = cm.exception
        #print "\n\n", e
        self.assertIn(msg, str(e))

    def clear_table(self):
        self.conn.run(self.table.delete())

    def do_insert(self, docs):
        self.expect(self.table.insert(docs), {'inserted': len(docs)})

    def test_arith(self):
        expect = self.expect
        fail = self.error_exec

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
        fail = self.error_exec

        expect(r.eq(3, 3), True)
        expect(r.eq(3, 4), False)

        expect(r.ne(3, 3), False)
        expect(r.ne(3, 4), True)

        expect(r.gt(3, 2), True)
        expect(r.gt(3, 3), False)

        expect(r.ge(3, 3), True)
        expect(r.ge(3, 4), False)

        expect(r.lt(3, 3), False)
        expect(r.lt(3, 4), True)

        expect(r.le(3, 3), True)
        expect(r.le(3, 4), True)
        expect(r.le(3, 2), False)

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

        self.error_exec(r.all(True, 3), "bool")
        self.error_exec(r.any(True, 4), "bool")

    def test_not(self):
        self.expect(r.not_(True), False)
        self.expect(r.not_(False), True)
        self.error_exec(r.not_(3), "bool")

    def test_let(self):
        self.expect(r.let(("x", 3), r.var("x")), 3)
        self.expect(r.let(("x", 3), ("x", 4), r.var("x")), 4)
        self.expect(r.let(("x", 3), ("y", 4), r.var("x")), 3)

        self.error_query(r.var("x"), "not in scope")

    def test_if(self):
        self.expect(r.if_(True, 3, 4), 3)
        self.expect(r.if_(False, 4, 5), 5)
        self.expect(r.if_(r.eq(3, 3), "foo", "bar"), "foo")

        self.error_exec(r.if_(5, 1, 2), "bool")

    def test_attr(self):
        self.expect(r.has({"foo": 3}, "foo"), True)
        self.expect(r.has({"foo": 3}, "bar"), False)

        self.expect(r.attr({"foo": 3}, "foo"), 3)
        self.error_exec(r.attr({"foo": 3}, "bar"), "missing")

        self.expect(r.attr({"a": {"b": 3}}, "a.b"), 3)

    def test_extend(self):
        self.expect(r.extend({"a": 5}, {"b": 3}), {"a": 5, "b": 3})
        self.expect(r.extend({"a": 5}, {"a": 3}), {"a": 3})
        self.expect(r.extend(r.extend({"a": 5, "b": 1}, {"a": 3}), {"b": 6}), {"a": 3, "b": 6})

        self.error_exec(r.extend(5, {"a": 3}), "object")
        self.error_exec(r.extend({"a": 5}, 5), "object")

    def test_array(self):
        expect = self.expect
        fail = self.error_exec

        expect(r.append([], 2), [2])
        expect(r.append([1], 2), [1, 2])
        fail(r.append(3, 0), "array")

        expect(r.add([1], [2]), [1, 2])
        expect(r.add([1, 2], []), [1, 2])
        fail(r.add([1], 1), "array")

        arr = range(10)
        expect(r.slice_(arr, 0, 3), arr[0: 3])
        expect(r.slice_(arr, 0, 0), arr[0: 0])
        expect(r.slice_(arr, 5, 15), arr[5: 15])
        expect(r.slice_(arr, 5, -3), arr[5: -3])
        expect(r.slice_(arr, -5, -3), arr[-5: -3])
        fail(r.slice_(1, 0, 0), "array")
        fail(r.slice_(arr, .5, 0), "integer")
        fail(r.slice_(arr, 0, 1.01), "integer")
        fail(r.slice_(arr, 5, 3), "greater")
        expect(r.slice_(arr, 5, None), arr[5:])
        expect(r.slice_(arr, None, 7), arr[:7])
        expect(r.slice_(arr, None, -2), arr[:-2])
        expect(r.slice_(arr, -2, None), arr[-2:])
        expect(r.slice_(arr, None, None), arr[:])

        expect(r.element(arr, 3), 3)
        expect(r.element(arr, -1), 9)
        fail(r.element(0, 0), "array")
        fail(r.element(arr, .1), "integer")
        fail(r.element([0], 1), "bounds")

        expect(r.length([]), 0)
        expect(r.length(arr), len(arr))
        fail(r.length(0), "array")

    def test_stream(self):
        expect = self.expect
        fail = self.error_exec
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
        fail = self.error_exec

        def limit(arr, count):
            return r.array(r.slice_(r.stream(arr), None, count))

        def skip(arr, offset):
            return r.array(r.slice_(r.stream(arr), offset, None))

        expect(limit([], 0), [])
        expect(limit([1, 2], 0), [])
        expect(limit([1, 2], 1), [1])
        expect(limit([1, 2], 5), [1, 2])
        fail(limit([], -1), "nonnegative")

        expect(skip([], 0), [])
        expect(skip([1, 2], 5), [])
        expect(skip([1, 2], 0), [1, 2])
        expect(skip([1, 2], 1), [2])

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
        fail = self.error_exec

        def order(arr, **kwargs):
            return r.array(r.stream(arr).orderby(**kwargs))

        docs = [{"id": 100 + n, "a": n, "b": n % 3} for n in range(10)]

        from operator import itemgetter as get

        expect(order(docs, a=True), sorted(docs, key=get('a')))
        expect(order(docs, a=False), sorted(docs, key=get('a'), reverse=True))

        self.clear_table()
        self.do_insert(docs)

        expect(self.table.orderby(a=True).array(), sorted(docs, key=get('a')))
        expect(self.table.orderby(a=False).array(), sorted(docs, key=get('a'), reverse=True))

        expect(self.table.filter({'b': 0}).orderby(a=True).array(),
               sorted(doc for doc in docs if doc['b'] == 0))

        expect(self.table.filter({'b': 0}).orderby(a=True).delete(),
               {'deleted': len(sorted(doc for doc in docs if doc['b'] == 0))})


    def test_table_insert(self):
        self.clear_table()

        docs = [{"a": 3, "b": 10, "id": 1}, {"a": 9, "b": -5, "id": 2}, {"a": 9, "b": 3, "id": 3}]

        self.do_insert(docs)

        for doc in docs:
            self.expect(self.table.find(doc['id']), doc)

        self.expect(r.array(self.table.distinct('a')), [3, 9])

        self.expect(self.table.filter({"a": 3}), [docs[0]])

        self.error_exec(self.table.filter({"a": r.add(self.table.count(), "")}), "numbers")

        self.error_exec(self.table.insert({"a": 3}), "id")

    def test_unicode(self):
        self.clear_table()

        doc = {"id": 100, "text": u"グルメ"}

        self.expect(self.table.insert(doc), {'inserted': 1})
        self.expect(self.table.find(100), doc)

    def test_view(self):
        self.clear_table()

        docs = [{"id": 1}, {"id": 2}]

        self.do_insert(docs)
        self.expect(self.table.limit(1).delete(), {'deleted': 1})

    def test_insertstream(self):
        self.clear_table()

        docs = [{"id": 100 + n, "a": n, "b": n % 3} for n in range(4)]
        self.expect(self.table.insert_stream(r.stream(docs)),
                    {'inserted': len(docs)})

        for doc in docs:
            self.expect(self.table.find(doc['id']), doc)

    def test_overload(self):
        self.clear_table()

        docs = [{"id": 100 + n, "a": n, "b": n % 3} for n in range(10)]
        self.do_insert(docs)

        def filt(exp, fn):
            self.expect(self.table.filter(exp).orderby(id=1), filter(fn, docs))

        filt(r['a'] == 5, lambda r: r['a'] == 5)
        filt(r['a'] != 5, lambda r: r['a'] != 5)
        filt(r['a'] < 5, lambda r: r['a'] < 5)
        filt(r['a'] <= 5, lambda r: r['a'] <= 5)
        filt(r['a'] > 5, lambda r: r['a'] > 5)
        filt(r['a'] >= 5, lambda r: r['a'] >= 5)

        filt(5 == r['a'], lambda r: 5 == r['a'])
        filt(5 != r['a'], lambda r: 5 != r['a'])
        filt(5 < r['a'], lambda r: 5 < r['a'])
        filt(5 <= r['a'], lambda r: 5 <= r['a'])
        filt(5 > r['a'], lambda r: 5 > r['a'])
        filt(5 >= r['a'], lambda r: 5 >= r['a'])

        filt(r['a'] == r['b'], lambda r: r['a'] == r['b'])
        filt(r['a'] == r['b'] + 1, lambda r: r['a'] == r['b'] + 1)
        filt(r['a'] == r['b'] + 1, lambda r: r['a'] == r['b'] + 1)

        expect = self.expect
        t = r.toTerm

        expect(-t(3), -3)
        expect(+t(3), 3)

        expect(t(3) + 4, 7)
        expect(t(3) - 4, -1)
        expect(t(3) * 4, 12)
        expect(t(3) / 4, 3./4)
        expect(t(3) % 2, 3 % 2)

        expect(3 + t(4), 7)
        expect(3 - t(4), -1)
        expect(3 * t(4), 12)
        expect(3 / t(4), 3./4)
        expect(3 % t(2), 3 % 2)

        expect((t(3) + 4) * -t(6) * (t(-5) + 3), 84)

    def test_getitem(self):
        expect = self.expect
        fail = self.error_exec
        t = r.toTerm

        arr = range(10)
        expect(t(arr)[:], arr[:])
        expect(t(arr)[2:], arr[2:])
        expect(t(arr)[:2], arr[:2])
        expect(t(arr)[-1:], arr[-1:])
        expect(t(arr)[:-1], arr[:-1])
        expect(t(arr)[3:5], arr[3:5])

        expect(t(arr)[3], arr[3])
        expect(t(arr)[-1], arr[-1])

        d = {'a': 3, 'b': 4}
        expect(t(d)['a'], d['a'])
        expect(t(d)['b'], d['b'])
        fail(t(d)['c'], 'missing attribute')

    def test_stream_getitem(self):
        arr = range(10)
        s = r.stream(arr)

        self.expect(s[:], arr[:])
        self.expect(s[3:], arr[3:])
        self.expect(s[:3], arr[:3])
        self.expect(s[4:6], arr[4:6])

        self.error_exec(s[4:'a'], "integer")

    def test_range(self):
        self.clear_table()

        docs = [{"id": n, "a": -n} for n in range(10)]
        self.do_insert(docs)

        self.expect(self.table.range(2, 3), docs[2:4])

    def test_js(self):
        self.expect(r.js('2'), 2)
        self.expect(r.js('2+2'), 4)
        self.expect(r.js('"cows"'), u"cows")
        self.expect(r.js('[1,2,3]'), [1,2,3])
        self.expect(r.js('{}'), {})
        self.expect(r.js('{a: "whee"}'), {u"a": u"whee"})
        self.expect(r.js('this'), {})

        self.expect(r.js(body='return 0;'), 0)

        self.error_exec(r.js('undefined'), "undefined")
        self.error_exec(r.js(body='return;'), "undefined")
        self.error_exec(r.js(body='var x = {}; x.x = x; return x;'), "cyclic datastructure")

    def test_js_vars(self):
        self.clear_table()
        names = "slava joe rntz rmmh tim".split()
        docs = [{'id': i, 'name': name} for i,name in enumerate(names)]

        self.expect(r.let(('x', 2), r.js('x')), 2)
        self.expect(r.let(('x', 2), ('y', 3), r.js('x + y')), 5)

        self.do_insert(docs)
        self.expect(self.table.map(r.var('x'), row='x'), docs) # sanity check

        self.expect(self.table.map(r.js('x'), row='x'), docs)
        self.expect(self.table.map(r.js('x.name'), row='x'), names)
        self.expect(self.table.filter(r.js('x.id > 2'), row='x'),
                    [x for x in docs if x['id'] > 2])
        self.expect(self.table.map(r.js('x.id + ": " + x.name'), row='x'),
                    ["%s: %s" % (x['id'], x['name']) for x in docs])

        self.expect(self.table, docs)
        self.expect(self.table.map(r.js('this')), docs)
        self.expect(self.table.map(r.js('this.name')), names)

    # def test_huge(self):
    #     self.clear_table()
    #     self.do_insert([{"id": 1}])

    #     increase = self.table.insert_stream(self.table.concat_map(r.stream(r.toTerm(range(10))).map(
    #         r.extend(r.var('r'), {'id': r.add(r.mul(r.var('r').attr('id'), 10), r.var('y'))}), row='y'), row='r'))

    #     self.expect(increase, {'inserted': '10'})

if __name__ == '__main__':
    unittest.main()

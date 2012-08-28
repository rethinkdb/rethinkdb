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

class RDBTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.conn = connect(
            os.environ.get('HOST', 'localhost'),
            int(os.environ.get('PORT', 12346+2010))
            )
        cls.table = table(os.environ.get('DB_NAME', 'Welcome-db') + "." + os.environ.get('TABLE_NAME', 'Welcome-rdb'))

    def expect(self, query, expected):
        try:
            res = self.conn.run(query)
            self.assertNotEqual(str(query), '')
            self.assertEqual(res, expected)
        except Exception, e:
            root_ast = rethinkdb.internal.p.Query()
            query._finalize_query(root_ast)
            print root_ast
            raise

    def error_query(self, query, msg):
        with self.assertRaises(BadQueryError) as cm:
            res = self.conn.run(query)
        e = cm.exception
        self.assertIn(msg, str(e))

    def error_exec(self, query, msg):
        with self.assertRaises(ExecutionError) as cm:
            res = self.conn.run(query)
        e = cm.exception
        #print "\n\n", e
        self.assertIn(msg, str(e))

    def clear_table(self):
        self.conn.run(self.table.delete())

    def do_insert(self, docs):
        self.expect(self.table.insert(docs), {'inserted': 1 if isinstance(docs, dict) else len(docs)})

    def test_arith(self):
        expect = self.expect
        fail = self.error_exec

        expect(expr(3) + 4, 7)
        expect(expr(3) + 4 + 5, 12)
        fail(expr(4) + [0], "number")

        expect(-expr(3), -3)
        expect(expr(0) - 3, -3)
        expect(expr(10) - 1 - 2, 7)
        fail(-expr([0]), "number")
        fail(expr(4) - [0], "number")

        expect(expr(4) * 5, 20)
        expect(expr(4) * 5 * 6, 120)
        fail(expr(4) * [0], "number")

        expect(expr(6) / 3, 2)
        expect(expr(12) / 3 / 4, 1)
        fail(expr(4) / [0], "number")

        expect(expr(4) % 3, 1)
        fail(expr([]) % 3, "number")
        fail(expr(3) % [], "number")

        expect((expr(3) + 4) * -expr(6) * (expr(-5) + 3), 84)

    def test_cmp(self):
        expect = self.expect
        fail = self.error_exec

        expect(expr(3) == 3, True)
        expect(expr(3) == 4, False)

        expect(expr(3) != 3, False)
        expect(expr(3) != 4, True)

        expect(expr(3) > 2, True)
        expect(expr(3) > 3, False)

        expect(expr(3) >= 3, True)
        expect(expr(3) >= 4, False)

        expect(expr(3) < 3, False)
        expect(expr(3) < 4, True)

        expect(expr(3) <= 3, True)
        expect(expr(3) <= 4, True)
        expect(expr(3) <= 2, False)

        expect(expr("asdf") == "asdf", True)
        expect(expr("asd") == "asdf", False)
        expect(expr("a") < "b", True)

        expect(expr(True) == True, True)
        expect(expr(False) < True, True)

        expect(expr(True) < 1, True)
        expect(expr(1) < "", True)
        expect(expr("") < [], True)

    def test_junctions(self):
        self.expect(expr(True) | False, True)
        self.expect(expr(False) | True, True)
        self.expect(expr(False) | False | True, True)

        self.expect(expr(True) & False, False)
        self.expect(expr(True) & True, True)
        self.expect(expr(True) & True & True, True)

        self.error_exec(expr(True) & 3, "bool")
        self.error_exec(expr(True) & 4, "bool")

    def test_not(self):
        self.expect(~expr(True), False)
        self.expect(~expr(False), True)
        self.error_exec(~expr(3), "bool")

    def test_let(self):
        self.expect(let(("x", 3), R("$x")), 3)
        self.expect(let(("x", 3), ("x", 4), R("$x")), 4)
        self.expect(let(("x", 3), ("y", 4), R("$x")), 3)

        self.error_query(R("$x"), "not in scope")

    def test_if(self):
        self.expect(if_then_else(True, 3, 4), 3)
        self.expect(if_then_else(False, 4, 5), 5)
        self.expect(if_then_else(expr(3) == 3, "foo", "bar"), "foo")

        self.error_exec(if_then_else(5, 1, 2), "bool")

    def test_attr(self):
        self.expect(expr({"foo": 3}).has_attr("foo"), True)
        self.expect(expr({"foo": 3}).has_attr("bar"), False)

        self.expect(expr({"foo": 3})["foo"], 3)
        self.error_exec(expr({"foo": 3})["bar"], "missing")

        self.expect(expr({"a": {"b": 3}})["a"]["b"], 3)

    def test_extend(self):
        self.expect(expr({"a": 5}).extend({"b": 3}), {"a": 5, "b": 3})
        self.expect(expr({"a": 5}).extend({"a": 3}), {"a": 3})
        self.expect(expr({"a": 5, "b": 1}).extend({"a": 3}).extend({"b": 6}), {"a": 3, "b": 6})

        self.error_exec(expr(5).extend({"a": 3}), "object")
        self.error_exec(expr({"a": 5}).extend(5), "object")

    def test_array(self):
        expect = self.expect
        fail = self.error_exec

        expect(expr([]).append(2), [2])
        expect(expr([1]).append(2), [1, 2])
        fail(expr(3).append(0), "array")

        expect(expr([1]) + [2], [1, 2])
        expect(expr([1, 2]) + [], [1, 2])
        fail(expr(1) + [1], "number")
        fail(expr([1]) + 1, "array")

        # TODO: Arrays accept negative indices, streams do not. That's kinda
        # confusing.

        arr = range(10)
        expect(expr(arr)[0:3], arr[0: 3])
        expect(expr(arr)[0:0], arr[0: 0])
        expect(expr(arr)[5:15], arr[5: 15])
        expect(expr(arr)[5:-3], arr[5: -3])
        expect(expr(arr)[-5:-3], arr[-5: -3])
        fail(expr(1)[0:0], "array")
        fail(expr(arr)[expr(.5):0], "integer")
        fail(expr(arr)[0:expr(1.01)], "integer")
        fail(expr(arr)[5:3], "greater")
        expect(expr(arr)[5:None], arr[5:])
        expect(expr(arr)[None:7], arr[:7])
        expect(expr(arr)[None:-2], arr[:-2])
        expect(expr(arr)[-2:None], arr[-2:])
        expect(expr(arr)[None:None], arr[:])

        expect(expr(arr)[3], 3)
        expect(expr(arr)[-1], 9)
        fail(expr(0)[0], "array")
        fail(expr(arr)[expr(.1)], "integer")
        fail(expr([0])[1], "bounds")

        expect(expr([]).length(), 0)
        expect(expr(arr).length(), len(arr))
        fail(expr(0).length(), "array")

    def test_stream(self):
        expect = self.expect
        fail = self.error_exec
        arr = range(10)

        expect(expr(arr).to_stream().to_array(), arr)

        expect(expr(arr).to_stream()[0], 0)
        expect(expr(arr).to_stream()[5], 5)
        fail(expr(arr).to_stream()[.4], "integer")
        fail(expr(arr).to_stream()[-1], "nonnegative")
        fail(expr([0]).to_stream()[1], "bounds")

    def test_stream_fancy(self):
        expect = self.expect
        fail = self.error_exec

        def limit(arr, count):
            return expr(arr).to_stream().limit(count).to_array()

        def skip(arr, offset):
            return expr(arr).to_stream().skip(offset).to_array()

        expect(limit([], 0), [])
        expect(limit([1, 2], 0), [])
        expect(limit([1, 2], 1), [1])
        expect(limit([1, 2], 5), [1, 2])
        fail(limit([], -1), "nonnegative")

        expect(skip([], 0), [])
        expect(skip([1, 2], 5), [])
        expect(skip([1, 2], 0), [1, 2])
        expect(skip([1, 2], 1), [2])

        def distinct(arr):
            return expr(arr).to_stream().distinct()

        expect(distinct([]), [])
        expect(distinct(range(10)*10), range(10))
        expect(distinct([1, 2, 3, 2]), [1, 2, 3])
        expect(distinct([True, 2, False, 2]), [True, 2, False])

        expect(
            expr([1, 2, 3]).to_stream().concat_map(fn("a", expr([R("$a") * 10, "test"]).to_stream())).to_array(),
            [10, "test", 20, "test", 30, "test"]
            )

    def test_grouped_map_reduce(self):
        purchases = [
            {"category": "food", "cost": 8},
            {"category": "food", "cost": 22},
            {"category": "food", "cost": 11},
            {"category": "entertainment", "cost": 6}
            ]
        self.expect(
            expr(purchases).to_stream().grouped_map_reduce(
                R("category"),
                R("cost"),
                0,
                fn("a", "b", R("$a") + R("$b"))
                ),
            {"food": 41, "entertainment": 6}
            )

    def test_reduce(self):
        expect(expr([1, 2, 3]).to_stream().reduce(0, fn("a", "b", R("$a") + R("$b"))), 6)
        expect(expr([1, 2, 3]).reduce(0, fn("a", "b", R("$a") + R("$b"))), 6)
        expect(expr([]).reduce(21, fn("a", "b", 0)), 21)

    def test_ordering(self):
        expect = self.expect
        fail = self.error_exec

        def order(arr, *args):
            return expr(arr).to_stream().orderby(*args)

        docs = [{"id": 100 + n, "a": n, "b": n % 3} for n in range(10)]

        from operator import itemgetter as get

        expect(order(docs, "a"), sorted(docs, key=get('a')))
        expect(order(docs, "-a"), sorted(docs, key=get('a'), reverse=True))

        self.clear_table()
        self.do_insert(docs)

        expect(self.table.orderby("a"), sorted(docs, key=get('a')))
        expect(self.table.orderby("-a"), sorted(docs, key=get('a'), reverse=True))

        expect(self.table.filter({'b': 0}).orderby("a"),
               sorted(doc for doc in docs if doc['b'] == 0))

        expect(self.table.filter({'b': 0}).orderby("a").delete(),
               {'deleted': len(sorted(doc for doc in docs if doc['b'] == 0))})

    def test_table_insert(self):
        self.clear_table()

        docs = [{"a": 3, "b": 10, "id": 1}, {"a": 9, "b": -5, "id": 2}, {"a": 9, "b": 3, "id": 3}]

        self.do_insert(docs)

        for doc in docs:
            self.expect(self.table.get(doc['id']), doc)

        self.expect(self.table.map(R("a")).distinct(), [3, 9])

        self.expect(self.table.filter({"a": 3}), [docs[0]])

        self.error_exec(self.table.filter({"a": self.table.length() + ""}), "numbers")

        self.error_exec(self.table.insert({"a": 3}), "id")

    def test_nonexistent_key(self):
        self.clear_table()
        self.expect(self.table.get(0), None)

    def test_unicode(self):
        self.clear_table()

        doc = {"id": 100, "text": u"グルメ"}

        self.do_insert(doc)
        self.expect(self.table.get(100), doc)

    def test_view(self):
        self.clear_table()

        docs = [{"id": 1}, {"id": 2}]

        self.do_insert(docs)
        self.expect(self.table.limit(1).delete(), {'deleted': 1})

    def test_insertstream(self):
        self.clear_table()

        docs = [{"id": 100 + n, "a": n, "b": n % 3} for n in range(4)]
        self.expect(self.table.insert_stream(expr(docs).to_stream()),
                    {'inserted': len(docs)})

        for doc in docs:
            self.expect(self.table.get(doc['id']), doc)

    def test_overload(self):
        self.clear_table()

        docs = [{"id": 100 + n, "a": n, "b": n % 3} for n in range(10)]
        self.do_insert(docs)

        def filt(exp, fn):
            self.expect(self.table.filter(exp).orderby("id"), filter(fn, docs))

        filt(R('a') == 5, lambda r: r['a'] == 5)
        filt(R('a') != 5, lambda r: r['a'] != 5)
        filt(R('a') < 5, lambda r: r['a'] < 5)
        filt(R('a') <= 5, lambda r: r['a'] <= 5)
        filt(R('a') > 5, lambda r: r['a'] > 5)
        filt(R('a') >= 5, lambda r: r['a'] >= 5)

        filt(5 == R('a'), lambda r: 5 == r['a'])
        filt(5 != R('a'), lambda r: 5 != r['a'])
        filt(5 < R('a'), lambda r: 5 < r['a'])
        filt(5 <= R('a'), lambda r: 5 <= r['a'])
        filt(5 > R('a'), lambda r: 5 > r['a'])
        filt(5 >= R('a'), lambda r: 5 >= r['a'])

        filt(R('a') == R('b'), lambda r: r['a'] == r['b'])
        filt(R('a') == R('b') + 1, lambda r: r['a'] == r['b'] + 1)
        filt(R('a') == R('b') + 1, lambda r: r['a'] == r['b'] + 1)

        expect = self.expect

        expect(-expr(3), -3)
        expect(+expr(3), 3)

        expect(expr(3) + 4, 7)
        expect(expr(3) - 4, -1)
        expect(expr(3) * 4, 12)
        expect(expr(3) / 4, 3./4)
        expect(expr(3) % 2, 3 % 2)

        expect(3 + expr(4), 7)
        expect(3 - expr(4), -1)
        expect(3 * expr(4), 12)
        expect(3 / expr(4), 3./4)
        expect(3 % expr(2), 3 % 2)

        expect((expr(3) + 4) * -expr(6) * (expr(-5) + 3), 84)

    def test_getitem(self):
        expect = self.expect
        fail = self.error_exec

        arr = range(10)
        expect(expr(arr)[:], arr[:])
        expect(expr(arr)[2:], arr[2:])
        expect(expr(arr)[:2], arr[:2])
        expect(expr(arr)[-1:], arr[-1:])
        expect(expr(arr)[:-1], arr[:-1])
        expect(expr(arr)[3:5], arr[3:5])

        expect(expr(arr)[3], arr[3])
        expect(expr(arr)[-1], arr[-1])

        d = {'a': 3, 'b': 4}
        expect(expr(d)['a'], d['a'])
        expect(expr(d)['b'], d['b'])
        fail(expr(d)['c'], 'missing attribute')

    def test_stream_getitem(self):
        arr = range(10)
        s = expr(arr).to_stream()

        self.expect(s[:], arr[:])
        self.expect(s[3:], arr[3:])
        self.expect(s[:3], arr[:3])
        self.expect(s[4:6], arr[4:6])

        self.error_exec(s[4:'a'], "integer")

    def test_range(self):
        self.clear_table()

        docs = [{"id": n, "a": "x" * n} for n in range(10)]
        self.do_insert(docs)

        self.expect(self.table.range(2, 3), docs[2:4])

    def test_js(self):
        self.expect(js('2'), 2)
        self.expect(js('2+2'), 4)
        self.expect(js('"cows"'), u"cows")
        self.expect(js('[1,2,3]'), [1,2,3])
        self.expect(js('{}'), {})
        self.expect(js('{a: "whee"}'), {u"a": u"whee"})
        self.expect(js('this'), {})

        self.expect(js(body='return 0;'), 0)

        self.error_exec(js('undefined'), "undefined")
        self.error_exec(js(body='return;'), "undefined")
        self.error_exec(js(body='var x = {}; x.x = x; return x;'), "cyclic datastructure")

    def test_js_vars(self):
        self.clear_table()
        names = "slava joe rntz rmmh tim".split()
        docs = [{'id': i, 'name': name} for i,name in enumerate(names)]

        self.expect(let(('x', 2), js('x')), 2)
        self.expect(let(('x', 2), ('y', 3), js('x + y')), 5)

        self.do_insert(docs)
        self.expect(self.table.map(fn("x", R('$x'))), docs) # sanity check

        self.expect(self.table.map(fn('x', js('x'))), docs)
        self.expect(self.table.map(fn('x', js('x.name'))), names)
        self.expect(self.table.filter(fn('x', js('x.id > 2'))),
                    [x for x in docs if x['id'] > 2])
        self.expect(self.table.map(fn('x', js('x.id + ": " + x.name'))),
                    ["%s: %s" % (x['id'], x['name']) for x in docs])

        self.expect(self.table, docs)
        self.expect(self.table.map(js('this')), docs)
        self.expect(self.table.map(js('this.name')), names)

    def test_updatemutate(self):
        self.clear_table()

        docs = [{"id": 100 + n, "a": n, "b": n % 3} for n in range(10)]
        self.do_insert(docs)

        self.expect(self.table.mutate(fn('x', R('$x'))), {"modified": len(docs), "deleted": 0})
        self.expect(self.table, docs)

        self.expect(self.table.update(None), {'updated': 0, 'skipped': 10, 'errors': 0})


    # def test_huge(self):
    #     self.clear_table()
    #     self.do_insert([{"id": 1}])

    #     increase = self.table.insert_stream(self.table.concat_map(r.stream(r.toTerm(range(10))).map(
    #         r.extend(r.var('r'), {'id': r.add(r.mul(r.var('r').attr('id'), 10), r.var('y'))}), row='y'), row='r'))

    #     self.expect(increase, {'inserted': '10'})

if __name__ == '__main__':
    unittest.main()

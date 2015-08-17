#!/usr/bin/env python

from __future__ import print_function

import contextlib, time

import utils, vcoptparse

r = utils.import_python_driver()

def option_parser_for_connect():
    op = vcoptparse.OptParser()
    op['address'] = vcoptparse.StringFlag('--address', 'localhost:28015')
    op['table'] = vcoptparse.StringFlag('--table')
    return op

@contextlib.contextmanager
def make_table_and_connection(opts):
    host, port = opts['address'].split(':')
    with r.connect(host, int(port)) as conn:
        db, table = opts['table'].split('.')
        yield (r.db(db).table(table), conn)

def insert_many(host="localhost", port=28015, database="test", table=None, count=10000, conn=None):
    if not conn:
        conn = r.connect(host, port)

    def gen(i):
        return {'val': "X" * (i % 100)}

    if isinstance(table, str) or isinstance(table, unicode):
        table = r.db(database).table(table)

    batch_size = 1000
    for start in range(0, count, batch_size):
        end = min(start + batch_size, count)
        res = table.insert([gen(i) for i in range(start, end)]).run(conn)
        if res.get('first_error'):
            raise Exception("Insert failed: " + res.get('first_error'))
        assert res['inserted'] == end - start

    print("inserted %d documents into %s" % (count, table))

#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import os, sys, random, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, "common")))
import rdb_workload_common
from line import *
from vcoptparse import *

op = rdb_workload_common.option_parser_for_connect()
op["count"] = IntFlag("--count", 100000)
opts = op.parse(sys.argv)

print_interval = 1
while print_interval * 100 < opts["count"]:
    print_interval *= 10

alphabet = "abcdefghijklmnopqrstuvwxyz"
pairs = []
for i in range(0, opts["count"]):
    key = random.choice(alphabet) + random.choice(alphabet) + random.choice(alphabet) + str(i)
    value = 'x' * (50 + 500 * i % 2)
    pairs.append((key, value))

with rdb_workload_common.make_table_and_connection(opts) as (table, conn):
    print "Creating test data"
    for i, (key, value) in enumerate(pairs):
        if (i + 1) % print_interval == 0:
            res = table.insert({'id':key, 'val': value}).run(conn)
            assert res['inserted'] == 1
            print i + 1,
        else:
            table.insert({'id':key, 'val': value}).run(conn, noreply=True)
    conn.noreply_wait()
    print
    print "Testing rget"
    res = iter(table.order_by(index='id').run(conn))
    for i, (key, value) in enumerate(sorted(pairs)):
        returned = res.next()
        expected = {'id': key, 'val': value}
        if returned != expected:
            raise Exception('Excepcted %s but got %s' % (expected, returned))
        if (i + 1) % print_interval == 0:
            print i + 1,
            sys.stdout.flush()
    print

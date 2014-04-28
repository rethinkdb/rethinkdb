#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import os, sys, random, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, "common")))
import rdb_workload_common
from line import *

key_padding = ''.zfill(20)
def gen_key(prefix, num):
    return prefix + key_padding + str(num).zfill(6)

value_padding = ''.zfill(25)
large_value_padding = ''.zfill(2000)
def gen_value(prefix, num):
    if num % 5 == 4:
        return prefix + large_value_padding + str(num).zfill(6)
    else:
        return prefix + value_padding + str(num).zfill(6)

def check_results(res, expected_count):
    count = len(res)
    if count < expected_count:
        raise ValueError("received less results than expected (expected: %d, got: %d)" % (expected_count, count))
    if count > expected_count:
        raise ValueError("received more results than expected (expected: %d, got: %d)" % (expected_count, count))

op = rdb_workload_common.option_parser_for_connect()
opts = op.parse(sys.argv)

foo_count = 100
fop_count = 1000

with rdb_workload_common.make_table_and_connection(opts) as (table, conn):
    print "Creating test data"
    data  = [{'id':gen_key('foo', i), 'val':gen_value('foo', i)} for i in range(0,foo_count)]
    data += [{'id':gen_key('fop', i), 'val':gen_value('fop', i)} for i in range(0,fop_count)]
    res = table.insert(data).run(conn)
    assert res['inserted'] == foo_count + fop_count

with rdb_workload_common.make_table_and_connection(opts) as (table, conn):
    print "Testing between"

    print "Checking simple between requests with open/closed boundaries"
    res = list(table.between(gen_key('foo', 0), gen_key('fop', 0)).run(conn))
    check_results(res, foo_count)

    res = list(table.between(gen_key('foo', 0), gen_key('fop', 0),
                             left_bound='open', right_bound='closed').run(conn))
    check_results(res, foo_count - 1 + 1)

    res = list(table.between(gen_key('foo', 0), gen_key('fop', 0),
                             right_bound='closed').run(conn))
    check_results(res, foo_count + 1)

    res = list(table.between(gen_key('foo', 0), gen_key('fop', 0),
                             left_bound='open').run(conn))
    check_results(res, foo_count - 1)

    print "Checking that between works when the boundares are not real keys"
    res = list(table.between(gen_key('a', 0), gen_key('fop', 0)).run(conn))
    check_results(res, foo_count)

    print "Checking larger number of results"
    res = list(table.between(gen_key('a', 0), gen_key('goo', 0)).run(conn))
    check_results(res, foo_count + fop_count)

    res = table.between(gen_key('foo', 0), gen_key('fop', 0)).order_by(index='id').run(conn)
    for i, kv in enumerate(res):
        expected_key = gen_key('foo', i)
        expected_value = gen_value('foo', i)
        if kv['id'] != expected_key:
            raise ValueError("received wrong key (expected: '%s', got: '%s')" % (expected_key, kv['id']))
        if kv['val'] != expected_value:
            raise ValueError("received wrong value (expected: '%s', got: '%s')" % (expected_value, kv['val']))

    print "Checking empty results being returned when no keys match"
    res = list(table.between(gen_key('a', 0), gen_key('b', 0)).run(conn))
    check_results(res, 0)

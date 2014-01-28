#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import os, sys, random, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, "common")))
import memcached_workload_common
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


def sock_readline(sock_file):
    ls = []
    while True:
        l = sock_file.readline()
        ls.append(l)
        if len(l) >= 2 and l[-2:] == '\r\n':
            break
    return ''.join(ls)

value_line = line("^VALUE\s+([^\s]+)\s+(\d+)\s+(\d+)\r\n$", [('key', 's'), ('flags', 'd'), ('length', 'd')])
def is_sorted_output(kvs):
    k = None
    for kv in kvs:
        if not k:
            k = kv['key']
            continue

        if k >= kv['key']:
            return False

        k = kv['key']
    return True

def get_results(s):
    res = []

    f = s.makefile()
    while True:
        l = sock_readline(f)
        if l == 'END\r\n':
            break
        val_def = value_line.parse_line(l)
        if not val_def:
            raise ValueError("received unexpected line from rget: %s" % l)
        val = sock_readline(f).rstrip()
        if len(val) != val_def['length']:
            raise ValueError("received value of unexpected length (expected %d, got %d: '%s')" % (val_def['length'], len(val), val))
            
        res.append({'key': val_def['key'], 'value': val})
    return res

def check_results(res, expected_count):
    count = len(res)
    if count < expected_count:
        raise ValueError("received less rget results than expected (expected: %d, got: %d)" % (expected_count, count))
    if count > expected_count:
        raise ValueError("received more rget results than expected (expected: %d, got: %d)" % (expected_count, count))
    if not is_sorted_output(res):
        raise ValueError("received unsorted rget output")

op = memcached_workload_common.option_parser_for_socket()
opts = op.parse(sys.argv)

foo_count = 100
fop_count = 1000
max_results = foo_count+fop_count

host, port = opts["address"]
with memcached_workload_common.MemcacheConnection(host, port) as mc:
    print "Creating test data"
    mc.set('z1', 'bar', time=1) # we expect it to expire before we check it later
    for i in range(0,foo_count):
        mc.set(gen_key('foo', i), gen_value('foo', i))
    for i in range(0,fop_count):
        mc.set(gen_key('fop', i), gen_value('fop', i))

with memcached_workload_common.make_socket_connection(opts) as s:
    print "Testing rget"

    print "Checking simple rget requests with open/closed boundaries"
    s.send('rget %s %s %d %d %d\r\n' % (gen_key('foo', 0), gen_key('fop', 0), 0, 1, max_results))
    res = get_results(s)
    check_results(res, foo_count)

    s.send('rget %s %s %d %d %d\r\n' % (gen_key('foo', 0), gen_key('fop', 0), 1, 0, max_results))
    res = get_results(s)
    check_results(res, foo_count - 1 + 1)

    s.send('rget %s %s %d %d %d\r\n' % (gen_key('foo', 0), gen_key('fop', 0), 1, 1, max_results))
    res = get_results(s)
    check_results(res, foo_count - 1)

    print "Checking that rget works when the boundares are not real keys"
    s.send('rget %s %s %d %d %d\r\n' % ('a', 'fop', 0, 0, max_results))
    res = get_results(s)
    check_results(res, foo_count)

    print "Checking larger number of results"
    s.send('rget %s %s %d %d %d\r\n' % ('a', gen_key('goo', 0), 0, 1, max_results))
    res = get_results(s)
    check_results(res, foo_count + fop_count)

    print "Checking simple paging"
    page_size = 13
    s.send('rget %s %s %d %d %d\r\n' % ('a', gen_key('goo', 0), 0, 1, page_size))
    res = get_results(s)
    check_results(res, page_size)

    print "Checking contiguous paging"
    page_size = 13
    res = []
    from_key = 'a'
    while True:
        s.send('rget %s %s %d %d %d\r\n' % (from_key, gen_key('fop', 0), 1, 1, page_size))
        cur_res = get_results(s)
        res.extend(cur_res)
        if len(res) < foo_count:
            check_results(cur_res, page_size)
            from_key = cur_res[-1]['key']
        else:
            check_results(cur_res, len(cur_res))
            break
    check_results(res, foo_count)

    for i in range(1,len(res)):
        kv = res[i]
        expected_key = gen_key('foo', i)
        expected_value = gen_value('foo', i)
        if kv['key'] != expected_key:
            raise ValueError("received wrong key (expected: '%s', got: '%s')" % (expected_key, kv['key']))
        if kv['value'] != expected_value:
            raise ValueError("received wrong value (expected: '%s', got: '%s')" % (expected_key, kv['value']))

    print "Checking empty results being returned when no keys match"
    s.send('rget %s %s %d %d %d\r\n' % ('a', 'b', 0, 0, max_results))
    res = get_results(s)
    check_results(res, 0)

    print "Checking that expired values are not returned"
    time.sleep(1)
    s.send('rget z0 z2 1 1 100\r\n')
    res = get_results(s)
    check_results(res, 0)

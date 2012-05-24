#!/usr/bin/python
import os, sys, random, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, "common")))
import workload_common
from line import *
from vcoptparse import *

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

def get_results(f):
    res = []
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

op = workload_common.option_parser_for_socket()
op["count"] = IntFlag("--count", 100000)
opts = op.parse(sys.argv)

print_interval = 1
while print_interval * 100 < opts["count"]:
    print_interval *= 10

with workload_common.make_socket_connection(opts) as s:
    f = s.makefile()
    print "Creating test data"
    for i in range(0, opts["count"]):
        key = gen_key('foo', i)
        value = gen_value('foo', i)
        if (i + 1) % print_interval == 0:
            s.send('set %s 0 0 %d\r\n%s\r\n' % (key, len(value), value))
            res = sock_readline(f)
            if res != "STORED\r\n":
                raise ValueError("Bad response to set: %r" % res)
            print i + 1,
            sys.stdout.flush()
        else:
            s.send('set %s 0 0 %d noreply\r\n%s\r\n' % (key, len(value), value))
    print
    print "Testing rget"
    s.send('rget null null -1 -1 %d\r\n' % (opts["count"] * 2))
    res = get_results(f)
    check_results(res, opts["count"])


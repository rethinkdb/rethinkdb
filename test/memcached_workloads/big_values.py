#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import memcached_workload_common

def readline(s):
    buf = ""
    while not buf.endswith("\r\n"):
        buf += s.recv(1)
    return buf

def abbreviate(s):
    if len(s) < 50: return repr(s)
    else: return repr(s[:25]) + "..." + repr(s[-25:])

def expect(s, string):
    msg = ""
    while len(msg) < len(string):
        msg += s.recv(len(string) - len(msg))
    if msg != string:
        raise ValueError("Didn't get what we expected: expected %s, got %s" % (abbreviate(string), abbreviate(msg)))

def expect_get_response(s, value):
    expect(s, "VALUE x 0 %d\r\n" % len(value))
    print " value decl...",
    expect(s, value + "\r\n")
    print " content...",
    expect(s, "END\r\n")
    print " ok."

def test_sizes_one_way(ap, x, y, s):
    print "Sending a %d-byte value..." % x,
    s.send(("set x 0 0 %d\r\n" % x) + ("a" * x) + "\r\n")

    max_legal_value_size = 1024 * 1024

    if x <= max_legal_value_size:
        expect(s, "STORED\r\n")
        print " getting...",
        s.send("get x\r\n")
        print " sent get...",
        expect_get_response(s, "a" * x)

        print "Now %sing upto length %d..." % (ap, y),
        s.send(("%s x 0 0 %d\r\n" % (ap, y - x)) + ("b" * (y - x)) + "\r\n")

        if y <= max_legal_value_size:
            expect(s, "STORED\r\n")
            print " getting...",
            s.send("get x\r\n")
            print " sent get...",
            expect_get_response(s, ("a" * x + "b" * (y - x) if ap == "append" else "b" * (y - x) + "a" * x))
        else:
            expect(s, "SERVER_ERROR object too large for cache\r\n")
            print " too big... ok."
    else:
        expect(s, "SERVER_ERROR object too large for cache\r\n")
        print " too big... ok."

def test_sizes_another_way(ap, x, y, s):
    test_sizes_one_way(ap, x, y, s)

def test_sizes(x, y, s):
    test_sizes_one_way("append", x, y, s)
    test_sizes_another_way("prepend", x, y, s)

op = memcached_workload_common.option_parser_for_socket()
opts = op.parse(sys.argv)

with memcached_workload_common.make_socket_connection(opts) as s:

    # 250 - the maximum small value
    # 251 - the minimum large buf (in a leaf node)
    # 4080 - the size of a large buf block (the largest large buf that uses a single block)
    # 8160 - twice the size of a large buf block
    # 65536 - 16-bit rollover
    # 73710 - netrecord causes some kind of weird failure at this point sometimes
    # (234 / 4) * 4080 - the biggest large value that uses one level
    # 10 * 1048576 - the maximum legal value size
    
    sizes = [250, 4079, 4080, 4081, 8160, 8161, (232 / 4) * 4080 - 1, (232 / 4) * 4080, (232 / 4) * 4080 + 1, 1048576, 10 * 1048577]

    for x in sizes:
        for y in sizes:
            if x < y:
                test_sizes(x, y, s)

    s.send("quit\r\n")

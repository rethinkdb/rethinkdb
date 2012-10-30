# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/env python
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import memcached_workload_common

# TODO: This readline function is copied and pasted from big_values.py.
def readline(s):
    buf = ""
    while not buf.endswith("\r\n"):
        buf += s.recv(1)
    return buf

# TODO: This expect function is copied and pasted from big_values.py.
def expect(s, string):
    msg = ""
    while len(msg) < len(string):
        msg += s.recv(len(string) - len(msg))
    if msg != string:
        raise ValueError("Didn't get what we expected: expected %s, got %s" % (string, msg));

def test_sizes(s, cmd, lo, hi):
    print ("testing un%s with %d .. %d" % (cmd, lo, hi))
    s.send("set x 0 0 %d\r\n" % lo + "a" * lo + "\r\n")
    msg = readline(s)
    if msg != "STORED\r\n":
        print ("Server responded with '%s', should have been STORED." % msg)
        raise ValueError("Initial large value of size %d not set.  Weird." % lo)

    # We send a malformed request, with an extra char!
    s.send("%s x 0 0 %d\r\n" % (cmd, (hi - lo)) + "b" * (hi - lo) + "b" + "\r\n")

    expect(s, "CLIENT_ERROR bad data chunk\r\n")

    s.send("get x\r\n")
    expect(s, "VALUE x 0 %d\r\n" % lo)
    expect(s, "a" * lo + "\r\n")
    expect(s, "END\r\n")

op = memcached_workload_common.option_parser_for_socket()
opts = op.parse(sys.argv)

with memcached_workload_common.make_socket_connection(opts) as s:

    sizes = [1, 100, 300, 1000, 8000, 700000]

    for (lo, hi) in [(x, y) for x in sizes for y in sizes if x < y]:
        for cmd in ["append", "prepend"]:
            test_sizes(s, cmd, lo, hi)

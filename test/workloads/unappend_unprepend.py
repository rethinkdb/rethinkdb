#!/usr/bin/env python
import sys, workload_common

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

op = workload_common.option_parser_for_socket()
opts = op.parse(sys.argv)

with workload_common.make_socket_connection(opts) as s:

    sizes = [1, 100, 300, 1000, 8000, 700000]

    for (lo, hi) in [(x, y) for x in sizes for y in sizes if x < y]:
        for cmd in ["append", "prepend"]:
            test_sizes(s, cmd, lo, hi)

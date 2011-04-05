#!/usr/bin/env python

from socket import *
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

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
        raise ValueError("Didn't get what we expected: expected %s, got %s" % (abbreviate(string), abbreviate(msg)));

def expect_get_response(s, value):
    expect(s, "VALUE x 0 %d\r\n" % len(value))
    print " value decl...",
    expect(s, value + "\r\n")
    print " content...",
    expect(s, "END\r\n")
    print " ok."

def test_sizes(x, y, s):
    print "Sending a %d-byte value..." % x,
    s.send(("set x 0 0 %d\r\n" % x) + ("a" * x) + "\r\n")

    max_legal_value_size = 1024 * 1024

    if x <= max_legal_value_size:
        expect(s, "STORED\r\n")
        print " getting...",
        s.send("get x\r\n")
        print " sent get...",
        expect_get_response(s, "a" * x)

        print "Appending upto length %d..." % y,
        s.send(("append x 0 0 %d\r\n" % (y - x)) + ("b" * (y - x)) + "\r\n")

        if y <= max_legal_value_size:
            expect(s, "STORED\r\n")
            print " getting...",
            s.send("get x\r\n")
            print " sent get...",
            expect_get_response(s, "a" * x + "b" * (y - x))
        else:
            expect(s, "SERVER_ERROR object too large for cache\r\n")
            print " too big... ok."
    else:
        expect(s, "SERVER_ERROR object too large for cache\r\n")
        print " too big... ok."

def test(opts, port, test_dir):

    sizes = range(249, 252) + range(4083, 4086) + range(8167, 8170) + range(65534, 65538) + range(73709, 73712) + range((234 / 4) * 4080 - 2, (234 / 4) * 4080 + 2) + range(1048575, 1048578)

    s = socket.socket()
    s.connect(("localhost", port))
    for x in sizes:
        for y in sizes:
            if x < y:
                test_sizes(x, y, s)

    s.send("quit\r\n")
    s.close()

if __name__ == "__main__":
    op = make_option_parser()
    opts = op.parse(sys.argv)
    auto_server_test_main(test, opts, timeout = 240)

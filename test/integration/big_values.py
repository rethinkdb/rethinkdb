#!/usr/bin/env python

from test_common import *
from socket import *

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

def test_value_of_size(size, port):

    print "Sending a %d-byte value..." % size,
    sys.stdout.flush()

    s = socket()
    s.connect(("localhost", port))

    s.send("set x 0 0 %d\r\n" % size + "a" * size + "\r\n")
    msg = readline(s)

    if msg == "STORED\r\n":
        print " getting...",
        s.send("get x\r\n");
        print " sent get...",
        expect(s, "VALUE x 0 %d\r\n" % size)
        print " value decl...",
        expect(s, "a" * size + "\r\n")
        print " content...",
        expect(s, "END\r\n")
        too_big = False
        print "ok."

    elif msg == "SERVER_ERROR object too large for cache\r\n":
        print "too big."
        too_big = True

    else:
        print
        raise ValueError("Something went wrong; sent a value of %d bytes and got %r" % (size, msg))

    s.send("quit")
    s.close()

    return too_big

def test_values_near_size(size, port):

    max_legal_value_size = 1024 * 1024

    for s in xrange(size - 5, size + 5):
        too_big = test_value_of_size(s, port)
        assert too_big == (s > max_legal_value_size)

def test(opts, port):

    # threshold for storing in node vs. storing in large bug
    test_values_near_size(250, port)

    # threshold for buffering vs. streaming
    test_values_near_size(10000, port)

    # uint16_t overflow.
    test_values_near_size(2 ** 16, port)

    # A common failure threshold when netrecord is turned on.
    test_values_near_size(73710, port)

    test_values_near_size(500000, port)


    # threshold for max legal value size
    test_values_near_size(1024 * 1024, port)

if __name__ == "__main__":
    op = make_option_parser()
    opts = op.parse(sys.argv)
    auto_server_test_main(test, opts, timeout = 60)

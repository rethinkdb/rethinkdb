# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
import os, sys, socket, random, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

def test_function(opts, port, test_dir):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", port))
    s.shutdown(socket.SHUT_RD)

    s.send("nonsense\r\n")

    for i in xrange(10, 100):
        v = str(i)
        s.send("set key 0 0 %d noreply\r\n%s\r\n" % (len(v), v))

    time.sleep(2)

    s.close()

    time.sleep(20)

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", port))

    s.send("get key\r\n")
    expected = "VALUE key 0 2\r\n99\r\nEND\r\n"
    received = s.recv(len(expected))
    if received != expected:
        print "Note if you get a wrong value here it's very possibly that the timeout was set too low, if you get no value then that's probably the actual issue popping up"
        print "Expected:", expected
        print "Received:", received
        raise ValueError("Bad value")

    s.close()

if __name__ == "__main__":
    op = make_option_parser()
    auto_server_test_main(test_function, op.parse(sys.argv))

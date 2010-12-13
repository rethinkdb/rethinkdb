#!/usr/bin/python
import os, sys, socket, random
from test_common import *
import time

n_appends = 100000

def test_function(opts, port):
    port = 11213
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", port))

    def send(str):
#print str
        s.send(str)

    key = 'fizz'
    val_chunk = 'buzz'

    send("set %s 0 0 %d noreply\r\n%s\r\n" % (key, len(val_chunk), val_chunk))

    for i in range(n_appends):
        time.sleep(.001)
        send("append %s 0 0 %d noreply\r\n%s\r\n" % (key, len(val_chunk), val_chunk))

    val = "buzz" * (n_appends + 1)

    send("get %s\r\n" % key)
    expected_res = "VALUE %s 0 %d\r\n%s\r\nEND\r\n" % (key, len(val), val)
    actual_val = s.recv(len(expected_res))
    if (expected_res != actual_val):
        print "Expected val: %s" % expected_res
        print "Incorrect val: %s", actual_val

if __name__ == "__main__":
    op = make_option_parser()
    auto_server_test_main(test_function, op.parse(sys.argv))

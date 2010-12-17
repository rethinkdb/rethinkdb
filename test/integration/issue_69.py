#!/usr/bin/python
import os, sys, socket, random
from test_common import *
import time

import pdb

def test_function(opts, *kwargs):
    make_test_dir()
    server = Server(opts)
    if not server.start():
        raise RuntimeError("Server failed to start")

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", server.port))

    # start sending large value and terminate the connection
    large_value_size = 1024
    s.send("set abc 0 0 %d\r\n" % large_value_size)
    time.sleep(3)   # wait to ensure that the server won't receive a flush while reading the sent string
    s.close()

    # this returns false if the test fails
    if not server.shutdown():
        raise TestFailure("Server failed to shutdown cleanly")

if __name__ == "__main__":
    op = make_option_parser()
    run_and_report(test_function, (op.parse(sys.argv), None), name = "server shutdown after large value connection termination")

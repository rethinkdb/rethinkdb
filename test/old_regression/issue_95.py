#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import os, sys, socket, random, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

n_ops = 10000

def test_function(opts, port, test_dir):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", port))

    s.send("Bullshit\r\n")

    for i in range(n_ops):
        s.send("set %d 0 0 %d noreply\r\n%d\r\n" % (i, len(str(i)), i))

    s.close()

if __name__ == "__main__":
    op = make_option_parser()
    auto_server_test_main(test_function, op.parse(sys.argv))

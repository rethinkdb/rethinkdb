# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
import random, time, sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

def test(opts, master, slave, test_dir):
    slave.shutdown()

    import socket
    for i in xrange(100):
        s = []
        for j in xrange(20):
            s += [socket.create_connection(("localhost", master.master_port))]
        map(lambda x: x.close(), s)

if __name__ == "__main__":
    op = make_option_parser()
    op = op.parse(sys.argv)
    master_slave_main(test, op, timeout = 100)

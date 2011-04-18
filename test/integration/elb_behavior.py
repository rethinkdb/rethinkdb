#!/usr/bin/python
import random, time, sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *
elb_ports = (find_unused_port(), find_unused_port())

def test(opts, server, repli_server, test_dir):
    s = socket.socket()
    try:
        s.connect(('localhost', server.elb_port))
    except socket.error:
        raise ValueError("master should be accepting elb connections but it isn't")

    s.close()
    couldnt_connect_to_slave = False
    try:
        s.connect(('localhost', repli_server.elb_port))
    except socket.error: #required to fail
        couldnt_connect_to_slave = True

    if not couldnt_connect_to_slave:
        raise ValueError("slave is accepting elb requests when it shouldn't be")

    s.close()

    server.shutdown()

    time.sleep(3) #give the slave a chance to realize the master is down

    try:
        s.connect(('localhost', repli_server.elb_port))
    except socket.error: #expected should not fail
        raise ValueError("slave should be accepting elb connections but it isn't")

    s.close()

if __name__ == "__main__":
    op = make_option_parser()
    op = op.parse(sys.argv)
    op["elb"] = True
    elb_test_main(test, op, timeout = (100 if not op.has_key("duration") else op["duration"] + 60))

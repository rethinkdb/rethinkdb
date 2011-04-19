#!/usr/bin/python
import random, time, sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

def test_connection(port, host = 'localhost'):
    s = socket.socket()
    try:
        s.connect((host, port))
    except socket.error:
        return False
    return True
    s.close()

def test(opts, server, repli_server, test_dir):
    if (not test_connection(server.elb_port)):
        raise ValueError("master should be accepting elb connections but it isn't")

    if (test_connection(repli_server.elb_port)):
        raise ValueError("slave is accepting elb requests when it shouldn't be")

    server.shutdown()

    time.sleep(10) #give the slave a chance to realize the master is down

    if (not test_connection(repli_server.elb_port)):
        raise ValueError("slave should be accepting elb connections on %d but it isn't" % repli_server.elb_port)

if __name__ == "__main__":
    op = make_option_parser()
    op = op.parse(sys.argv)
    op["elb"] = True
    elb_test_main(test, op, timeout = (100 if not op.has_key("duration") else op["duration"] + 60))

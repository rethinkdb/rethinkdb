# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
import os, sys, socket, random
import time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

n_ops = 10000

def test_function(opts, port, test_dir):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", port))

    def assert_on_socket(string):
        res = s.recv(len(string))
        if string != res:
            print "Got: %s, expect %s" % (res, string)
        assert string == res

    s.send("set fizz 0 0 4\r\nbuzz\r\n")
    assert_on_socket('STORED\r\n')

    s.send('delete fizz fizz fizz fizz\r\n')
    assert_on_socket('ERROR\r\n')

    s.send('get fizz\r\n')
    assert_on_socket('VALUE fizz 0 4\r\nbuzz\r\nEND\r\n')

    s.close()

if __name__ == "__main__":
    op = make_option_parser()
    auto_server_test_main(test_function, op.parse(sys.argv))

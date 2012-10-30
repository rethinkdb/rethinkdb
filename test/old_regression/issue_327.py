#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import os, sys, socket, random
import time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

def test_function(opts, server, test_dir):
    

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", server.port))
    
    def assert_on_socket(string):
        res = s.recv(len(string))
        if string != res:
            print "Got: %s, expect %s" % (res, string)
        assert string == res

    s.send("set a 0 0 300\r\nAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n")
    assert_on_socket('STORED\r\n')
    
    s.send('get a\r\n')
    assert_on_socket('VALUE a 0 300\r\nAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\nEND\r\n')

    s.close()
    
    # Restart the server.
    server.shutdown()
    server.start()

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", server.port))

    s.send("add a 0 0 1\r\nA\r\n")
    assert_on_socket('NOT_STORED\r\n')
    
    # In issue #327, this failed because setting to a large value didn't load the data into the cache,
    # but instead left the cache buffers uninitialized. Usually that's ok, because the new value will
    # invalidate (delete) the old large value buffers anyway. If the set operation fails (like with add),
    # the uninitializes buffers remained in the cache, causing the following get to return garbage:
    
    s.send('get a\r\n')
    assert_on_socket('VALUE a 0 300\r\nAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\nEND\r\n')

    s.close()

if __name__ == "__main__":
    op = make_option_parser()
    opts = op.parse(sys.argv)
    assert opts["auto"]

    test_dir = TestDir()
    server = Server(opts, test_dir=test_dir)
    server.start()

    try:
        run_with_timeout(test_function, args=(opts, server), timeout = adjust_timeout(opts, 60), test_dir=test_dir)
    except Exception, e:
        test_failure = e
    else:
        test_failure = None

    server.shutdown()
    if test_failure: raise test_failure


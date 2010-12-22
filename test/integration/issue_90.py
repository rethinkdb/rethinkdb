#!/usr/bin/python
import os, sys, socket, random
from test_common import *
import time

n_ops = 10000

def test_function(opts, port):
    port = 11213
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", port))

    def assert_on_socket(string):
        assert string == s.recv(len(string))

    s.send("set fizz 0 0 4\r\nbuzz\r\n")
    assert_on_socket('STORED\r\n')

    s.send('delete fizz fizz fizz fizz\r\n')
    assert_on_socket('SERVER_ERROR functionality not supported\r\n')

    s.send('get fizz\r\n')
    assert_on_socket('VALUE fizz 0 4\r\nbuzz\r\nEND\r\n')

    s.close()

if __name__ == "__main__":
    op = make_option_parser()
    auto_server_test_main(test_function, op.parse(sys.argv))

# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
import os, sys, socket, random, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, "common")))
from test_common import *

def test_function(opts, port, test_dir):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", port))

    # start sending large value and terminate the connection
    large_value_size = 1024
    s.send("set abc 0 0 %d\r\n" % large_value_size)
    time.sleep(3)   # wait to ensure that the server won't receive a flush while reading the sent string
    s.close()

if __name__ == "__main__":
    op = make_option_parser()
    auto_server_test_main(test_function, op.parse(sys.argv))

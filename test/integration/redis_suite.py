#!/usr/bin/python
import os, subprocess, sys 
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

redis_tests_dir = os.path.join(os.path.dirname(__file__), "redis_suite/tests")

def test(opts, port, test_dir):
    assert False

if __name__ == "__main__":
    op = make_option_parser()
    op["suite-test"] = StringFlag("--suite-test")
    op["auto"] = True
    auto_server_test_main(test, op.parse(sys.argv), timeout=800)

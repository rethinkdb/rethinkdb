#!/usr/bin/python
import os, subprocess, sys
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

ec2 = 5

def test(opts, port, test_dir):

    # Figure out where the memcached scripts are located
    memcached_suite_dir = os.path.join(os.path.dirname(__file__), "memcached_suite")

    # The memcached test scripts now get the port as an environment variable (instead of running
    # the server themselves).
    os.environ["RUN_PORT"] = str(port)
    os.environ["PERLLIB"] = os.path.join(memcached_suite_dir, "lib") + ":" + os.getenv("PERLLIB", "")

    proc = subprocess.Popen(os.path.join(memcached_suite_dir, opts["suite-test"]))
    suite_test_succeeded = proc.wait() == 0
    assert suite_test_succeeded

if __name__ == "__main__":
    op = make_option_parser()
    op["suite-test"] = StringFlag("--suite-test")
    auto_server_test_main(test, op.parse(sys.argv), timeout=800)

#!/usr/bin/python
import os, subprocess, sys
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

ec2 = 5

def test(opts, port, test_dir):
    # The test scripts now get the port as an environment variable (instead of running the server themselves).
    os.environ["RUN_PORT"] = str(port)
    os.environ["PERLLIB"] = os.path.abspath(os.getcwd()) + "/integration/memcached_suite/lib:" + os.getenv("PERLLIB", "")
    proc = subprocess.Popen(opts["suite-test"])
    assert proc.wait() == 0

if __name__ == "__main__":
    op = make_option_parser()
    op["suite-test"] = StringFlag("--suite-test")
    auto_server_test_main(test, op.parse(sys.argv), timeout=360)

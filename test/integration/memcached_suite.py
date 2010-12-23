#!/usr/bin/python
import os, subprocess
from test_common import *

def test(opts, port):
    # The test scripts now get the port as an environment variable (instead of running the server themselves).
    os.environ["RUN_PORT"] = str(port)
    os.environ["PERLLIB"] = os.path.abspath(os.getcwd()) + "/integration/memcached_suite/lib:" + os.getenv("PERLLIB", "")
    proc = subprocess.Popen(opts["suite-test"])
    assert proc.wait() == 0

if __name__ == "__main__":
    op = make_option_parser()
    op["suite-test"] = StringFlag("--suite-test")
    auto_server_test_main(test, op.parse(sys.argv), timeout=120)

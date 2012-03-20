#!/usr/bin/python
import os, sys, subprocess, workload_common
from vcoptparse import *

op = workload_common.option_parser_for_socket()
op["suite-test"] = PositionalArg()
opts = op.parse(sys.argv)

# Figure out where the memcached scripts are located
memcached_suite_dir = os.path.join(os.path.dirname(__file__), "memcached_suite")

# The memcached test scripts now get the port as an environment variable
# (instead of running the server themselves).
assert opts["host"] in ["localhost", "127.0.0.1"]
os.environ["RUN_PORT"] = str(opts["port"])
os.environ["PERLLIB"] = os.path.join(memcached_suite_dir, "lib") + ":" + os.getenv("PERLLIB", "")

proc = subprocess.Popen(os.path.join(memcached_suite_dir, opts["suite-test"]))
suite_test_succeeded = proc.wait() == 0
assert suite_test_succeeded

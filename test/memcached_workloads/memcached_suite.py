# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
import os, sys
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import subprocess, memcached_workload_common
from vcoptparse import *

op = memcached_workload_common.option_parser_for_socket()
op["suite-test"] = PositionalArg()
opts = op.parse(sys.argv)

# Figure out where the memcached scripts are located
memcached_suite_dir = os.path.join(os.path.dirname(__file__), "memcached_suite")

# The memcached test scripts now get the port as an environment variable
# (instead of running the server themselves).
assert opts["address"][0] in ["localhost", "127.0.0.1"]
os.environ["RUN_PORT"] = str(opts["address"][1])
os.environ["PERLLIB"] = os.path.join(memcached_suite_dir, "lib") + ":" + os.getenv("PERLLIB", "")

subprocess.check_call(os.path.join(memcached_suite_dir, opts["suite-test"]))

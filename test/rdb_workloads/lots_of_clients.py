#!/usr/bin/python
# Copyright 2013 RethinkDB, all rights reserved.

# Command line arguments:
# --workload foo     - the workload to be run $NUM_CLIENTS times simultaneously.


# Environment variables:
# NUM_CLIENTS: the number of clients to run (default = 1000)
# Other environment variables might be needed for your workloads.

import shlex
import subprocess
import sys
import os
from vcoptparse import *

op = OptParser()
op["workload"] = StringFlag("--workload")
op["pre-workload"] = StringFlag("--pre-workload", "true")
opts = op.parse(sys.argv)

workload = opts["workload"]
pre_workload = opts["pre_workload"]

num_clients = int(os.environ.get('NUM_CLIENTS', 1000))

children = []

child = subprocess.Popen(shlex.split(pre_workload))
child.wait()

for i in xrange(num_clients):
    popen = subprocess.Popen(shlex.split(workload))
    children.append(popen)

for child in children:
    if 0 != child.wait():
        raise "a child process failed"





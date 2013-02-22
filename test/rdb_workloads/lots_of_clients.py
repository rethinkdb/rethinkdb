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

if sys.argv[1] == '--workload':
    workload = sys.argv[2]
else:
    raise "Invalid command line: expecting '%s --workload <workload>'" % sys.argv[0]

num_clients = int(os.environ.get('NUM_CLIENTS', 1000))

children = []

for i in xrange(num_clients):
    popen = subprocess.Popen(shlex.split(workload))
    children.append(popen)

for child in children:
    if 0 != child.wait():
        raise "a child process failed"





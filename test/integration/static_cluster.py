#!/usr/bin/python
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin
from vcoptparse import *

op = OptParser()
op["workload"] = PositionalArg()
op["num-nodes"] = IntFlag("--num-nodes", 3)
opts = op.parse(sys.argv)

cluster = http_admin.Cluster()
for i in xrange(opts["num-nodes"]):
    cluster.add_machine(name = "x" * (i + 1))

cluster.add_memcached_namespace(name = "Test Namespace")

print cluster
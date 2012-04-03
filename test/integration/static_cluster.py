#!/usr/bin/python
import sys, os, subprocess, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin
from workload_runner import create_workload
from vcoptparse import *

op = OptParser()
op["workload"] = PositionalArg()
op["protocol"] = StringFlag("--protocol", "memcached")
op["num-nodes"] = IntFlag("--num-nodes", 3)
opts = op.parse(sys.argv)

if "$HOST" not in opts["workload"] or "$PORT" not in opts["workload"]:
    print "Workload should be a command-line with '$HOST' and '$PORT' in place "
    print "of the hostname and port of the server. Did you forget to escape "
    print "the dollar signs? Workload was %r." % opts["workload"]
    sys.exit(1)

cluster = http_admin.Cluster()
for i in xrange(opts["num-nodes"]):
    cluster.add_machine(name = "x" * (i + 1))

datacenter = cluster.add_datacenter(name = "Test Datacenter")

for machine in cluster.machines:
    cluster.move_server_to_datacenter(machine, datacenter)

namespace = cluster.add_namespace(protocol = opts["protocol"], name = "Test Namespace", primary = datacenter)
host, port = cluster.get_namespace_host(namespace)

time.sleep(3)

command_line = opts["workload"].replace("$HOST", host).replace("$PORT", str(port))
# Assume that there is at most 2 phases to the workload
workload = create_workload(command_line)

import signal
import pdb

def sighandler(sig):
	print "blah"
	pdb.set_trace()

signal.signal(signal.SIGUSR1, sighandler)

try:
	workload.run(0)
	# Nothing to do for this scenario
	workload.finish()
	sys.exit(0)
except RuntimeError:
	sys.exit(1)


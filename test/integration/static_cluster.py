#!/usr/bin/python
import sys, os, subprocess, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin
from vcoptparse import *

op = OptParser()
op["workload"] = PositionalArg()
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

namespace = cluster.add_namespace(name = "Test Namespace", primary = datacenter)

print cluster

port = cluster.compute_port(namespace, next(cluster.machines.iterkeys()))

command_line = opts["workload"].replace("$HOST", "localhost").replace("$PORT", str(port))
print "Running", repr(command_line)+"..."
start_time = time.time()
try:
    subprocess.check_call(command_line, shell = True)
except subprocess.CalledProcessError:
    end_time = time.time()
    print "Failed (%d seconds)" % (end_time - start_time)
else:
    end_time = time.time()
    print "Done (%d seconds)" % (end_time - start_time)

#!/usr/bin/python
import sys, os, subprocess, time
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

datacenter = cluster.add_datacenter(name = "Test Datacenter")

for machine in cluster.machines:
    cluster.move_server_to_datacenter(machine, datacenter)

namespace = cluster.add_namespace(name = "Test Namespace", primary = datacenter)

port = cluster.compute_port(namespace, next(cluster.machines.iterkeys()))

time.sleep(5)

command_line = opts["workload"]
new_environ = os.environ.copy()
new_environ["HOST"] = "localhost"
new_environ["PORT"] = str(port)

print "Running %r with HOST=%r and PORT=%r..." % (command_line, new_environ["HOST"], new_environ["PORT"])
start_time = time.time()
try:
    subprocess.check_call(command_line, shell = True, env = new_environ)
except subprocess.CalledProcessError:
    end_time = time.time()
    print "Failed (%d seconds)" % (end_time - start_time)
    sys.exit(1)
else:
    end_time = time.time()
    print "Done (%d seconds)" % (end_time - start_time)

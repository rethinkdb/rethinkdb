#!/usr/bin/python
import sys, os, subprocess, time, signal
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin
from vcoptparse import *

op = OptParser()
op["workload"] = PositionalArg()
op["num-nodes"] = IntFlag("--num-nodes", 3)
op["timeout"] = IntFlag("--timeout", 600)
opts = op.parse(sys.argv)

cluster = http_admin.Cluster()
for i in xrange(opts["num-nodes"]):
    cluster.add_machine(name = "x" * (i + 1))

datacenter = cluster.add_datacenter(name = "Test Datacenter")

for machine in cluster.machines:
    cluster.move_server_to_datacenter(machine, datacenter)

namespace = cluster.add_namespace(name = "Test Namespace", primary = datacenter)

port = cluster.compute_port(namespace, next(cluster.machines.iterkeys()))

wait_time = 5
print "Waiting %d seconds for cluster to configure itself..." % wait_time
time.sleep(wait_time)

command_line = opts["workload"]
new_environ = os.environ.copy()
new_environ["HOST"] = "localhost"
new_environ["PORT"] = str(port)

print "Running %r with HOST=%r and PORT=%r..." % (command_line, new_environ["HOST"], new_environ["PORT"])
start_time = time.time()
subp = subprocess.Popen(command_line, shell = True, env = new_environ)
while time.time() < start_time + opts["timeout"]:
    if not cluster.is_alive():
        print "Cluster crashed (%d seconds)" % (time.time() - start_time)
        try:
            subp.send_signal(signal.SIGKILL)
        except OSError:
            pass
        sys.exit(1)
    elif subp.poll() == 0:
        print "Done (%d seconds)" % (time.time() - start_time)
        sys.exit(0)
    elif subp.poll() is not None:
        print "Failed (%d seconds)" % (time.time() - start_time)
        sys.exit(1)
    time.sleep(1)
print "Timed out (%d seconds)" % opts["timeout"]
try:
    subp.send_signal(signal.SIGKILL)
except OSError:
    pass
sys.exit(1)

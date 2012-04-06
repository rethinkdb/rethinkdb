#!/usr/bin/python
import scenario
import sys
import time
from vcoptparse import *

op = scenario.get_default_option_parser()
op["protocol"] = StringFlag("--protocol", "memcached")
op["num-nodes"] = IntFlag("--num-nodes", 3)
opts = op.parse(sys.argv)

def initialize_cluster(cluster):
	for i in xrange(opts["num-nodes"]):
		cluster.add_machine(name = "x" * (i + 1))

	datacenter = cluster.add_datacenter(name = "Test Datacenter")

	for machine in cluster.machines:
		cluster.move_server_to_datacenter(machine, datacenter)

	namespace = cluster.add_namespace(protocol = opts["protocol"], name = "Test Namespace", primary = datacenter)
	time.sleep(8)
	return cluster.get_namespace_host(namespace)

scenario.run_scenario(opts, initialize_cluster)


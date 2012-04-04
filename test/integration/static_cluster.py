#!/usr/bin/python
import scenario
import sys
from vcoptparse import *

def initialize_cluster(cluster, opts):
	for i in xrange(opts["num-nodes"]):
		cluster.add_machine(name = "x" * (i + 1))

	datacenter = cluster.add_datacenter(name = "Test Datacenter")

	for machine in cluster.machines:
		cluster.move_server_to_datacenter(machine, datacenter)

	namespace = cluster.add_namespace(protocol = opts["protocol"], name = "Test Namespace", primary = datacenter)
	return cluster.get_namespace_host(namespace)

op = scenario.get_default_option_parser()
op["protocol"] = StringFlag("--protocol", "memcached")
op["num-nodes"] = IntFlag("--num-nodes", 3)

scenario.run_scenario(op.parse(sys.argv), initialize_cluster)


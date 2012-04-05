#!/usr/bin/python
import scenario
import sys
import time
from vcoptparse import *

op = scenario.get_default_option_parser()
op["protocol"] = StringFlag("--protocol", "memcached")
#op["num-nodes"] = IntFlag("--num-nodes", 2)
opts = op.parse(sys.argv)

datacenter = None
namespace = None
secondary = None

def initialize_cluster(cluster):
#	for i in xrange(opts["num-nodes"]):
#		cluster.add_machine(name = "x" * (i + 1))
	primary = cluster.add_machine(name = "x" * (1))
	global secondary
	secondary = cluster.add_machine(name = "x" * (2))

	global datacenter
	datacenter = cluster.add_datacenter(name = "Test Datacenter")

	cluster.move_server_to_datacenter(primary, datacenter)

	global namespace
	namespace = cluster.add_namespace(protocol = opts["protocol"], name = "Test Namespace", primary = datacenter)
	time.sleep(5)
	return cluster.get_namespace_host(namespace)

def change_affinities(cluster):
	cluster.move_server_to_datacenter(secondary, datacenter)
	cluster.set_namespace_affinities(namespace, { datacenter: 1 })
	time.sleep(2)
	return cluster.get_namespace_host(namespace)

scenario.run_scenario(opts, initialize_cluster, [change_affinities])


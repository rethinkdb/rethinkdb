#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, time, tempfile
rethinkdb_root = os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir))
sys.path.append(os.path.join(rethinkdb_root, "test", "common"))
import http_admin, driver
sys.path.append(os.path.join(rethinkdb_root, "test", "scenarios"))
import workload_runner
from vcoptparse import *

op = OptParser()
op["workload"] = PositionalArg()
op["timeout"] = IntFlag("--timeout", 600)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    print "Starting cluster..."
    num_nodes = 2
    files = [driver.Files(metacluster, db_path = "db-%d" % i, log_path = "create-output-%d" % i)
        for i in xrange(num_nodes)]
    processes = [driver.Process(
            cluster,
            files[i],
            log_path = "serve-output-%d" % i,
            executable_path = driver.find_rethinkdb_executable("debug"))
        for i in xrange(num_nodes)]
    time.sleep(10)
    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in processes])
    dc = http.add_datacenter()
    for machine_id in http.machines:
        http.move_server_to_datacenter(machine_id, dc)
    ns = http.add_namespace(protocol = "memcached", primary = dc)
    time.sleep(10)
    host, port = driver.get_namespace_host(ns.port, processes)
    cluster.check()

    workload_ports = workload_runner.MemcachedPorts(
        "localhost",
        processes[0].http_port,
        ns.port + processes[0].port_offset)
    workload_runner.run("memcached", opts["workload"], workload_ports, opts["timeout"])
    cluster.check()

    print "Splitting into two shards..."
    http.add_namespace_shard(ns, "t")
    time.sleep(10)
    cluster.check()

    print "Merging shards together again..."
    http.remove_namespace_shard(ns, "t")
    time.sleep(10)
    cluster.check()

    cluster.check_and_stop()

#!/usr/bin/python
import sys, os, time, tempfile
import workload_runner
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver
from vcoptparse import *

op = OptParser()
op["mode"] = IntFlag("--mode", "debug")
op["workload1"] = PositionalArg()
op["workload2"] = PositionalArg()
op["workload3"] = PositionalArg()
op["workload4"] = PositionalArg()
op["timeout"] = IntFlag("--timeout", 600)
opts = op.parse(sys.argv)

with driver.Metacluster(driver.find_rethinkdb_executable(opts["mode"])) as metacluster:
    cluster = driver.Cluster(metacluster)
    print "Starting cluster..."
    num_nodes = 2
    files = [driver.Files(metacluster, db_path = "db-%d" % i, log_path = "create-output-%d" % i)
        for i in xrange(num_nodes)]
    processes = [driver.Process(cluster, files[i], log_path = "serve-output-%d" % i)
        for i in xrange(num_nodes)]
    time.sleep(3)
    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in processes])
    dc = http.add_datacenter()
    for machine_id in http.machines:
        http.move_server_to_datacenter(machine_id, dc)
    ns = http.add_namespace(protocol = "memcached", primary = dc)
    time.sleep(10)
    host, port = http.get_namespace_host(ns)
    cluster.check()

    workload_runner.run(opts["workload1"], host, port, opts["timeout"])
    cluster.check()

    print "Splitting into two shards..."
    http.add_namespace_shard(ns, "t")
    time.sleep(10)
    cluster.check()

    workload_runner.run(opts["workload2"], host, port, opts["timeout"])
    cluster.check()

    print "Increasing replication factor..."
    http.set_namespace_affinities(ns, {dc: 1})
    time.sleep(10)
    cluster.check()

    workload_runner.run(opts["workload3"], host, port, opts["timeout"])
    cluster.check()

    print "Merging shards together again..."
    http.remove_namespace_shard(ns, "t")
    time.sleep(10)
    cluster.check()

    workload_runner.run(opts["workload4"], host, port, opts["timeout"])
    cluster.check()

    cluster.check_and_stop()

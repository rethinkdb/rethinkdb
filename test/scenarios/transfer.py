#!/usr/bin/python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner
from vcoptparse import *

op = OptParser()
op["mode"] = IntFlag("--mode", "debug")
op["workload1"] = PositionalArg()
op["workload2"] = PositionalArg()
op["timeout"] = IntFlag("--timeout", 600)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)

    print "Starting cluster..."
    files1 = driver.Files(metacluster, db_path = "db-first")
    process1 = driver.Process(cluster, files1, executable_path = driver.find_rethinkdb_executable(opts["mode"]))
    time.sleep(3)

    print "Creating namespace..."
    http1 = http_admin.ClusterAccess([("localhost", process1.http_port)])
    dc = http1.add_datacenter()
    http1.move_server_to_datacenter(files1.machine_name, dc)
    ns = http1.add_namespace(protocol = "memcached", primary = dc)
    time.sleep(10)

    host, port = http1.get_namespace_host(ns)
    workload_runner.run(opts["workload1"], host, port, opts["timeout"])

    print "Bringing up new server..."
    files2 = driver.Files(metacluster, db_path = "db-second")
    process2 = driver.Process(cluster, files2, executable_path = driver.find_rethinkdb_executable(opts["mode"]))
    time.sleep(3)
    http1.update_cluster_data()
    http1.move_server_to_datacenter(files2.machine_name, dc)
    http1.set_namespace_affinities(ns, {dc: 1})
    http1.check_no_issues()

    print "Waiting for backfill..."
    time.sleep(30)

    print "Shutting down old server..."
    process1.check_and_stop()
    http2 = http_admin.ClusterAccess([("localhost", process2.http_port)])
    http2.declare_machine_dead(files1.machine_name)
    http2.set_namespace_affinities(ns.name, {dc.name: 0})
    http2.check_no_issues()
    time.sleep(10)

    host, port = http2.get_namespace_host(ns.name)
    workload_runner.run(opts["workload2"], host, port, opts["timeout"])

    cluster.check_and_stop()

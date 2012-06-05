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
    files = driver.Files(metacluster)
    process = driver.Process(cluster, files, executable_path = driver.find_rethinkdb_executable(opts["mode"]))
    process.wait_until_started_up()
    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", process.http_port)])
    dc = http.add_datacenter()
    http.move_server_to_datacenter(http.machines.keys()[0], dc)
    ns = http.add_namespace(protocol = "memcached", primary = dc)
    http.wait_until_blueprint_satisfied(ns)
    host, port = http.get_namespace_host(ns)
    workload_runner.run(opts["workload1"], host, port, opts["timeout"])
    print "Restarting server..."
    process.check_and_stop()
    process2 = driver.Process(cluster, files)
    process2.wait_until_started_up()
    http.wait_until_blueprint_satisfied(ns)
    workload_runner.run(opts["workload2"], host, port, opts["timeout"])
    cluster.check_and_stop()

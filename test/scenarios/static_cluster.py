#!/usr/bin/python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner
from vcoptparse import *

op = OptParser()
op["num-nodes"] = IntFlag("--num-nodes", 3)
op["mode"] = IntFlag("--mode", "debug")
op["workload"] = PositionalArg()
op["timeout"] = IntFlag("--timeout", 600)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    print "Starting cluster..."
    processes = [driver.Process(cluster, driver.Files(metacluster, db_path = "db-%d" % i), executable_path = driver.find_rethinkdb_executable(opts["mode"]), log_path = "serve-output-%d" % i)
        for i in xrange(opts["num-nodes"])]
    time.sleep(3)
    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in processes])
    dc = http.add_datacenter()
    for machine_id in http.machines:
        http.move_server_to_datacenter(machine_id, dc)
    ns = http.add_namespace(protocol = "memcached", primary = dc)
    time.sleep(10)
    host, port = http.get_namespace_host(ns)
    workload_runner.run(opts["workload"], host, port, opts["timeout"])
    cluster.check_and_close()


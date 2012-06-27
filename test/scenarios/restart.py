#!/usr/bin/python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner, scenario_common
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
op["workload1"] = PositionalArg()
op["workload2"] = PositionalArg()
op["timeout"] = IntFlag("--timeout", 600)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix  = scenario_common.parse_mode_flags(opts)
    print "Starting cluster..."
    files = driver.Files(metacluster, executable_path = executable_path, command_prefix = command_prefix)
    process = driver.Process(cluster, files, executable_path = executable_path, command_prefix = command_prefix)
    process.wait_until_started_up()
    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", process.http_port)])
    dc = http.add_datacenter()
    http.move_server_to_datacenter(http.machines.keys()[0], dc)
    ns = http.add_namespace(protocol = "memcached", primary = dc)
    http.wait_until_blueprint_satisfied(ns)
    host, port = driver.get_namespace_host(ns.port, [process])
    workload_runner.run(opts["workload1"], host, port, opts["timeout"])
    print "Restarting server..."
    process.check_and_stop()
    process2 = driver.Process(cluster, files)
    process2.wait_until_started_up()
    http.wait_until_blueprint_satisfied(ns)
    workload_runner.run(opts["workload2"], host, port, opts["timeout"])
    cluster.check_and_stop()

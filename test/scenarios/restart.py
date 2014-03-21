#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
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
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Starting cluster..."
    files = driver.Files(metacluster, log_path = "create-output",
                         executable_path = executable_path, command_prefix = command_prefix)
    process = driver.Process(cluster, files,
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    process.wait_until_started_up()
    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", process.http_port)])
    dc = http.add_datacenter()
    http.move_server_to_datacenter(http.machines.keys()[0], dc)
    ns = scenario_common.prepare_table_for_workload(opts, http, primary = dc)
    http.wait_until_blueprint_satisfied(ns)
    workload_ports = scenario_common.get_workload_ports(opts, ns, [process])
    workload_runner.run(opts["protocol"], opts["workload1"], workload_ports, opts["timeout"])
    print "Restarting server..."
    process.check_and_stop()
    process2 = driver.Process(cluster, files,
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    process2.wait_until_started_up()
    # NOTE (daniel): I don't know why but for some weird reason wait_until_blueprint_satisfied
    # shuts down the server. Really no idea why... As a quick and dirty work-around, let's just
    # sleep 5 seconds
    time.sleep(5)
    # http.wait_until_blueprint_satisfied(ns)
    workload_runner.run(opts["protocol"], opts["workload2"], workload_ports, opts["timeout"])
    print "Shutting down..."
    cluster.check_and_stop()

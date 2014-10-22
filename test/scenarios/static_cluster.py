#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

import sys, os, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin, scenario_common, workload_runner
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
op["use-proxy"] = BoolFlag("--use-proxy")
op["num-nodes"] = IntFlag("--num-nodes", 3)
op["num-shards"] = IntFlag("--num-shards", 2)
op["workload"] = PositionalArg()
op["timeout"] = IntFlag("--timeout", 1200)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Starting cluster..."
    processes = [driver.Process(
    		cluster,
            driver.Files(metacluster, db_path="db-%d" % i, console_output="create-output-%d" % i, command_prefix=command_prefix),
            console_output="serve-output-%d" % i, command_prefix=command_prefix, extra_options=serve_options)
    	for i in xrange(opts["num-nodes"])]
    if opts["use-proxy"]:
        processes.append(driver.ProxyProcess(cluster, 'proxy-logfile', console_output='proxy-output', command_prefix=command_prefix, extra_options=serve_options))
    for process in processes:
        process.wait_until_started_up()

    print "Creating table..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in processes])
    dc = http.add_datacenter()
    for machine_id in http.machines:
        http.move_server_to_datacenter(machine_id, dc)
    ns = scenario_common.prepare_table_for_workload(http, primary = dc)
    for i in xrange(opts["num-shards"] - 1):
        http.add_table_shard(ns, chr(ord('a') + 26 * i // opts["num-shards"]))
    http.wait_until_blueprint_satisfied(ns)

    workload_ports = scenario_common.get_workload_ports(ns, processes if not opts["use-proxy"] else [proxy_process])
    workload_runner.run(opts["workload"], workload_ports, opts["timeout"])

    cluster.check_and_stop()

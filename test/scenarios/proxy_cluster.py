# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner, scenario_common
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
op["workload"] = PositionalArg()
op["timeout"] = IntFlag("--timeout", 600)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Starting cluster..."
    serve_files = driver.Files(metacluster, db_path = "db", log_path = "create-output",
                               executable_path = executable_path, command_prefix = command_prefix)
    serve_process = driver.Process(cluster, serve_files, log_path = "serve-output",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    proxy_process = driver.ProxyProcess(cluster, 'proxy-logfile', log_path = 'proxy-output',
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    processes = [serve_process, proxy_process]
    for process in processes:
        process.wait_until_started_up()

    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", proxy_process.http_port)])
    dc = http.add_datacenter()
    for machine_id in http.machines:
        http.move_server_to_datacenter(machine_id, dc)
    ns = scenario_common.prepare_table_for_workload(opts, http, primary = dc)
    http.wait_until_blueprint_satisfied(ns)

    workload_ports = scenario_common.get_workload_ports(opts, ns, [proxy_process])
    workload_runner.run(opts["protocol"], opts["workload"], workload_ports, opts["timeout"])

    cluster.check_and_stop()


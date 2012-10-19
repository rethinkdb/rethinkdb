#!/usr/bin/python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner, scenario_common
from vcoptparse import *

op = OptParser()
workload_runner.prepare_option_parser_for_split_or_continuous_workload(op)
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    print "Starting cluster..."
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    files1 = driver.Files(metacluster, db_path = "db-1", log_path = "create-output-1",
                          executable_path = executable_path, command_prefix = command_prefix)
    process1 = driver.Process(cluster, files1, log_path = "serve-output-1",
                              executable_path = executable_path, command_prefix = command_prefix,
                              extra_options = serve_options)
    files2 = driver.Files(metacluster, db_path = "db-2", log_path = "create-output-2",
                          executable_path = executable_path, command_prefix = command_prefix)
    process2 = driver.Process(cluster, files2, log_path = "serve-output-2",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    process1.wait_until_started_up()
    process2.wait_until_started_up()

    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in [process1, process2]])
    dc1 = http.add_datacenter()
    http.move_server_to_datacenter(process1.files.machine_name, dc1)
    dc2 = http.add_datacenter()
    http.move_server_to_datacenter(process2.files.machine_name, dc2)
    ns = scenario_common.prepare_table_for_workload(opts, http, primary = dc1, affinities = {dc1: 0, dc2: 1})
    http.wait_until_blueprint_satisfied(ns)
    cluster.check()
    http.check_no_issues()

    workload_ports = scenario_common.get_workload_ports(opts, ns, [process1, process2])
    with workload_runner.SplitOrContinuousWorkload(opts, opts["protocol"], workload_ports) as workload:
        workload.run_before()
        cluster.check()
        http.check_no_issues()
        workload.check()
        print "Changing the primary..."
        http.set_namespace_affinities(ns, {dc1: 1, dc2: 1})
        http.move_namespace_to_datacenter(ns, dc2)
        http.set_namespace_affinities(ns, {dc1: 1, dc2: 0})
        http.wait_until_blueprint_satisfied(ns)
        cluster.check()
        http.check_no_issues()
        workload.run_after()

    http.check_no_issues()
    cluster.check_and_stop()

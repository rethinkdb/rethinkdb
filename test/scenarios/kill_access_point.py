#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner, scenario_common
from vcoptparse import *

op = OptParser()
op["workload"] = PositionalArg()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    print "Starting cluster..."
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    database_machine_files = driver.Files(metacluster, db_path = "db-database", log_path = "create-db-database-output",
                                          executable_path = executable_path, command_prefix = command_prefix)
    database_machine = driver.Process(cluster, database_machine_files, log_path = "serve-output-database",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    access_machine_files = driver.Files(metacluster, db_path = "db-access", log_path = "create-db-access-output",
                                        executable_path = executable_path, command_prefix = command_prefix)
    access_machine = driver.Process(cluster, access_machine_files, log_path = "serve-output-access",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    database_machine.wait_until_started_up
    access_machine.wait_until_started_up()

    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", database_machine.http_port)])
    database_dc = http.add_datacenter()
    http.move_server_to_datacenter(database_machine.files.machine_name, database_dc)
    other_dc = http.add_datacenter()
    http.move_server_to_datacenter(access_machine.files.machine_name, other_dc)
    ns = scenario_common.prepare_table_for_workload(opts, http, primary = database_dc)
    http.wait_until_blueprint_satisfied(ns)
    cluster.check()
    http.check_no_issues()

    workload_ports = scenario_common.get_workload_ports(opts, ns, [access_machine])
    with workload_runner.ContinuousWorkload(opts["workload"], opts["protocol"], workload_ports) as workload:
        workload.start()
        print "Running workload for 10 seconds..."
        time.sleep(10)
        cluster.check()
        http.check_no_issues()
        print "Killing the access machine..."
        access_machine.close()
        # Don't bother stopping the workload, just exit and it will get killed

    http.declare_machine_dead(access_machine.files.machine_name)
    http.check_no_issues()
    database_machine.check()
    print "The other machine dosn't seem to have crashed."
